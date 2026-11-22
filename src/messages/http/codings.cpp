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

#include <algorithm>
#include <charconv>
#include <variant>
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

encoder::encoder(const codings& value) NOEXCEPT
{
    const auto add = [&](coding item) NOEXCEPT
    {
        if (item == coding::gzip)
            stages_.emplace_back(std::in_place_type<gzip_deflater>);
        else
            stages_.emplace_back(std::in_place_type<zlib_deflater>);
    };

    std::for_each(value.content.begin(), value.content.end(), add);
    std::for_each(value.transfer.begin(), value.transfer.end(), add);
}

// The output of each stage is the input of the next.
void encoder::write(data_chunk& out, const data_slice& data) NOEXCEPT
{
    data_chunk carry{ data.begin(), data.end() };
    for (auto& stage: stages_)
    {
        data_chunk next{};
        std::visit([&](auto& coder) NOEXCEPT
        {
            coder.write(next, carry);
        }, stage);

        carry = std::move(next);
    }

    out.insert(out.end(), carry.begin(), carry.end());
}

void encoder::flush(data_chunk& out) NOEXCEPT
{
    data_chunk carry{};
    for (auto& stage: stages_)
    {
        data_chunk next{};
        std::visit([&](auto& coder) NOEXCEPT
        {
            if (!carry.empty())
                coder.write(next, carry);

            coder.flush(next);
        }, stage);

        carry = std::move(next);
    }

    out.insert(out.end(), carry.begin(), carry.end());
}

// Decoding.
// ----------------------------------------------------------------------------

decoder::decoder(const codings& value, size_t limit) NOEXCEPT
  : limit_(limit)
{
    const auto add = [&](coding item, http_error_t fault) NOEXCEPT
    {
        if (item == coding::gzip)
            stages_.push_back({ coder{ std::in_place_type<gzip_inflater> },
                fault });
        else
            stages_.push_back({ coder{ std::in_place_type<zlib_inflater> },
                fault });
    };

    for (auto it = value.transfer.rbegin(); it != value.transfer.rend(); ++it)
        add(*it, http_error_t::bad_transfer_encoding);

    for (auto it = value.content.rbegin(); it != value.content.rend(); ++it)
        add(*it, http_error_t::bad_content_encoding);
}

bool decoder::done() const NOEXCEPT
{
    return std::all_of(stages_.begin(), stages_.end(),
        [](const stage& item) NOEXCEPT
        {
            const auto ended = std::visit([](const auto& coder) NOEXCEPT
            {
                return coder.done();
            }, item.value);

            return ended && item.input.empty();
        });
}

// First stage decodes in, each other the output of preceding stage.
// Each stage pauses past limit, and last also at step.
void decoder::write(data_chunk& out, size_t& consumed, const data_slice& in,
    size_t step, code& ec) NOEXCEPT
{
    consumed = zero;
    const auto start = out.size();
    const auto room = is_zero(limit_) ? zero :
        add1(limit_ - std::min(limit_, total_));
    for (size_t index{}; index < stages_.size(); ++index)
    {
        auto& item = stages_[index];
        const auto first = is_zero(index);
        const auto last = index == sub1(stages_.size());
        auto& sink = last ? out : stages_[add1(index)].input;
        const data_slice source = first ? in : data_slice(item.input);

        auto pause = room;
        if (last && !is_zero(step))
            pause = is_zero(room) ? step : std::min(step, room);

        size_t used{};
        const auto valid = std::visit([&](auto& coder) NOEXCEPT
        {
            return coder.write(sink, used, source, pause);
        }, item.value);

        if (!valid)
        {
            ec = to_http_code(item.fault);
            return;
        }

        if (first)
            consumed = used;
        else
            item.input.erase(item.input.begin(),
                std::next(item.input.begin(), used));
    }

    total_ += out.size() - start;
    if (!is_zero(limit_) && total_ > limit_)
    {
        ec = to_http_code(http_error_t::body_limit);
        return;
    }

    ec.clear();
}

// One-shot.
// ----------------------------------------------------------------------------

data_chunk encode(const data_slice& body, const codings& value) NOEXCEPT
{
    data_chunk out{};
    encoder coder{ value };
    coder.write(out, body);
    coder.flush(out);
    return out;
}

data_chunk decode(const data_slice& body, const codings& value, size_t limit,
    code& ec) NOEXCEPT
{
    data_chunk out{};
    size_t offset{};
    decoder coder{ value, limit };
    while (!coder.done())
    {
        size_t used{};
        const auto before = out.size();
        const auto begin = std::next(body.begin(), offset);
        coder.write(out, used, data_slice(begin, body.end()), zero, ec);
        if (ec)
            return {};

        offset += used;
        if (is_zero(used) && out.size() == before)
            break;
    }

    if (offset != body.size() || !coder.done())
    {
        const auto fault = value.content.empty() ?
            http_error_t::bad_transfer_encoding :
            http_error_t::bad_content_encoding;

        ec = to_http_code(fault);
        return {};
    }

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
