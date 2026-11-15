/**
 * Copyright (c) 2011-2026 libbitcoin developers
 *
 * This file is part of libbitcoin.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/messages/http/codings.hpp>

#include <charconv>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/coding.hpp>
#include <bitcoin/network/messages/http/enums/field.hpp>
#include <bitcoin/network/messages/http/fields.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace system;
using namespace network::error;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

constexpr auto chunked_token = "chunked";

// Appends the known codings of the list, false if any is unknown.
static bool append_codings(std::vector<coding>& out,
    const string_list& names) NOEXCEPT
{
    for (const auto& name: names)
    {
        if (name.empty())
            continue;

        const auto value = to_coding(name);
        if (value == coding::unknown)
            return false;

        if (value != coding::identity)
            out.push_back(value);
    }

    return true;
}

codings to_codings(const fields& header, code& ec) NOEXCEPT
{
    codings out{};
    for (const auto& entry: header)
    {
        if (entry.id == field::content_encoding)
        {
            const auto names = split(entry.value, { "," }, http_whitespace);
            if (!append_codings(out.content, names))
            {
                ec = to_http_code(http_error_t::bad_content_encoding);
                return {};
            }
        }
        else if (entry.id == field::transfer_encoding)
        {
            auto names = split(entry.value, { "," }, http_whitespace);
            if (!names.empty() && iequals(names.back(), chunked_token))
                names.pop_back();

            if (!append_codings(out.transfer, names))
            {
                ec = to_http_code(http_error_t::bad_transfer_encoding);
                return {};
            }
        }
    }

    ec.clear();
    return out;
}

// Encoding.
// ----------------------------------------------------------------------------

static data_chunk encode(const data_slice& body, coding value) NOEXCEPT
{
    switch (value)
    {
        case coding::gzip:
            return gzip_deflate(body);
        case coding::deflate:
            return zlib_deflate(body);
        case coding::identity:
        case coding::unknown:
        default:
            return to_chunk(body);
    }
}

static bool decode(data_chunk& out, const data_slice& body, coding value,
    size_t limit) NOEXCEPT
{
    switch (value)
    {
        case coding::gzip:
            return gzip_inflate(out, body, limit);
        case coding::deflate:
            return zlib_inflate(out, body, limit);
        case coding::identity:
        case coding::unknown:
        default:
            return false;
    }
}

data_chunk encode(const data_slice& body, const codings& value) NOEXCEPT
{
    auto out = to_chunk(body);
    for (const auto item: value.content)
        out = encode(out, item);

    for (const auto item: value.transfer)
        out = encode(out, item);

    return out;
}

data_chunk decode(const data_slice& body, const codings& value, size_t limit,
    code& ec) NOEXCEPT
{
    auto out = to_chunk(body);
    for (auto it = value.transfer.rbegin(); it != value.transfer.rend(); ++it)
    {
        data_chunk decoded{};
        if (!decode(decoded, out, *it, limit))
        {
            ec = to_http_code(http_error_t::bad_transfer_encoding);
            return {};
        }

        out = std::move(decoded);
    }

    for (auto it = value.content.rbegin(); it != value.content.rend(); ++it)
    {
        data_chunk decoded{};
        if (!decode(decoded, out, *it, limit))
        {
            ec = to_http_code(http_error_t::bad_content_encoding);
            return {};
        }

        out = std::move(decoded);
    }

    ec.clear();
    return out;
}

// Negotiation.
// ----------------------------------------------------------------------------

// Quality in thousandths, "0" [ "." 0*3DIGIT ] / "1" [ "." 0*3("0") ]
// (rfc9110 12.4.2), false if invalid.
static bool to_quality(size_t& out, const std::string_view& text) NOEXCEPT
{
    constexpr size_t maximum = 1000;
    constexpr size_t maximum_digits = 3;
    constexpr std_array<size_t, add1(maximum_digits)> scales
    {
        1000, 100, 10, 1
    };

    if (text.empty() || (text.front() != '0' && text.front() != '1'))
        return false;

    const auto whole = text.front() == '1' ? maximum : zero;
    if (text.size() == one)
    {
        out = whole;
        return true;
    }

    const auto fraction = text.substr(two);
    if (text[one] != '.' || fraction.size() > maximum_digits)
        return false;

    size_t value{};
    if (!fraction.empty())
    {
        const auto last = std::next(fraction.data(), fraction.size());
        const auto [next, error] = std::from_chars(fraction.data(), last,
            value);

        if (error != std::errc{} || next != last)
            return false;
    }

    value = whole + value * scales.at(fraction.size());
    if (value > maximum)
        return false;

    out = value;
    return true;
}

coding accepted_coding(const fields& request) NOEXCEPT
{
    constexpr size_t unlisted = max_size_t;
    constexpr size_t preferred = 1000;
    constexpr size_t least = 1;
    constexpr std::string_view quality_name{ "q=" };

    auto present = false;
    auto listed_any = unlisted;
    auto listed_gzip = unlisted;
    auto listed_deflate = unlisted;
    auto listed_identity = unlisted;
    for (const auto& entry: request)
    {
        if (entry.id != field::accept_encoding)
            continue;

        present = true;
        for (const auto& item: split(entry.value, { "," }, http_whitespace))
        {
            const auto parts = split(item, { ";" }, http_whitespace);
            if (parts.empty() || parts.front().empty())
                continue;

            auto valid = true;
            auto quality = preferred;
            for (auto part = std::next(parts.begin()); part != parts.end();
                ++part)
            {
                const std::string_view text{ *part };
                if (iequals(text.substr(zero, quality_name.size()),
                    quality_name))
                    valid = to_quality(quality,
                        text.substr(quality_name.size()));
            }

            if (!valid)
                continue;

            if (parts.front() == "*")
            {
                listed_any = quality;
                continue;
            }

            switch (to_coding(parts.front()))
            {
                case coding::gzip:
                    listed_gzip = quality;
                    break;
                case coding::deflate:
                    listed_deflate = quality;
                    break;
                case coding::identity:
                    listed_identity = quality;
                    break;
                case coding::unknown:
                default:
                    break;
            }
        }
    }

    // Without the field any coding is acceptable, identity is the default.
    if (!present)
        return coding::identity;

    // Unlisted codings take the wildcard quality, if listed.
    const auto wildcard = [&](size_t quality, size_t otherwise) NOEXCEPT
    {
        if (quality != unlisted)
            return quality;

        return listed_any != unlisted ? listed_any : otherwise;
    };

    // Unlisted identity remains acceptable, as the least preferred.
    const auto gzip_quality = wildcard(listed_gzip, zero);
    const auto deflate_quality = wildcard(listed_deflate, zero);
    const auto identity_quality = wildcard(listed_identity, least);

    if (is_nonzero(gzip_quality) && gzip_quality >= deflate_quality &&
        gzip_quality >= identity_quality)
        return coding::gzip;

    if (is_nonzero(deflate_quality) && deflate_quality >= identity_quality)
        return coding::deflate;

    if (is_nonzero(identity_quality))
        return coding::identity;

    return coding::unknown;
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
