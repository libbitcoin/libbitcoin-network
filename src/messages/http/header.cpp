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
#include <bitcoin/network/messages/http/header.hpp>

#include <charconv>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/fields.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

constexpr auto close_token = "close";
constexpr auto keep_alive_token = "keep-alive";
constexpr auto chunked_token = "chunked";

// The comma separated list without the matching tokens.
template <typename Predicate>
static std::string filter_tokens(const std::string_view& list,
    Predicate&& matches) NOEXCEPT
{
    std::string out{};
    for (const auto& token: split(list, { "," }, http_whitespace))
    {
        if (token.empty() || matches(token))
            continue;

        if (!out.empty())
            out += ", ";

        out += token;
    }

    return out;
}

static std::string append_token(const std::string& list,
    const std::string_view& token) NOEXCEPT
{
    if (has_token(list, token))
        return list;

    return list.empty() ? std::string{ token } : list + ", " +
        std::string{ token };
}

// header_base
// ----------------------------------------------------------------------------

header_base::header_base(uint32_t version) NOEXCEPT
  : version_(version)
{
}

uint32_t header_base::version() const NOEXCEPT
{
    return version_;
}

void header_base::version(uint32_t value) NOEXCEPT
{
    version_ = value;
}

bool header_base::keep_alive() const NOEXCEPT
{
    const auto it = find(field::connection);
    if (version_ < version_1_1)
        return it != end() && has_token(it->value, keep_alive_token);

    return it == end() || !has_token(it->value, close_token);
}

void header_base::keep_alive(bool value) NOEXCEPT
{
    const auto list = (*this)[field::connection];
    const auto is = [](const std::string_view& token) NOEXCEPT
    {
        return [token](const std::string_view& item) NOEXCEPT
        {
            return iequals(item, token);
        };
    };

    std::string out{};
    if (version_ < version_1_1)
    {
        if (value)
            out = append_token(filter_tokens(list, is(close_token)),
                keep_alive_token);
        else
            out = filter_tokens(list, [&](const std::string_view& item)
                NOEXCEPT
                {
                    return is(close_token)(item) || is(keep_alive_token)(item);
                });
    }
    else
    {
        if (value)
            out = filter_tokens(list, [&](const std::string_view& item)
                NOEXCEPT
                {
                    return is(close_token)(item) || is(keep_alive_token)(item);
                });
        else
            out = append_token(filter_tokens(list, is(keep_alive_token)),
                close_token);
    }

    if (out.empty())
        erase(field::connection);
    else
        set(field::connection, out);
}

bool header_base::chunked() const NOEXCEPT
{
    const auto codings = split((*this)[field::transfer_encoding], { "," },
        http_whitespace);

    return !codings.empty() && iequals(codings.back(), chunked_token);
}

void header_base::chunked(bool value) NOEXCEPT
{
    const auto list = (*this)[field::transfer_encoding];
    auto out = filter_tokens(list, [](const std::string_view& item) NOEXCEPT
    {
        return iequals(item, chunked_token);
    });

    if (value)
    {
        erase(field::content_length);
        out = out.empty() ? std::string{ chunked_token } :
            out + ", " + chunked_token;
    }

    if (out.empty())
        erase(field::transfer_encoding);
    else
        set(field::transfer_encoding, out);
}

std::optional<uint64_t> header_base::content_length() const NOEXCEPT
{
    const auto it = find(field::content_length);
    if (it == end())
        return {};

    uint64_t value{};
    const auto& text = it->value;
    const auto last = std::next(text.data(), text.size());
    const auto [next, ec] = std::from_chars(text.data(), last, value);
    if (ec != std::errc{} || next != last || text.empty())
        return {};

    return value;
}

void header_base::content_length(
    const std::optional<uint64_t>& value) NOEXCEPT
{
    chunked(false);
    if (value.has_value())
        set(field::content_length, serialize(value.value()));
    else
        erase(field::content_length);
}

// header<true>
// ----------------------------------------------------------------------------

header<true>::header() NOEXCEPT
  : header(verb::unknown, {}, version_1_1)
{
}

header<true>::header(verb method, const std::string_view& target,
    uint32_t version) NOEXCEPT
  : header_base(version), method_(method), target_(target)
{
}

verb header<true>::method() const NOEXCEPT
{
    return method_;
}

void header<true>::method(verb value) NOEXCEPT
{
    method_ = value;
    method_string_.clear();
}

std::string_view header<true>::method_string() const NOEXCEPT
{
    return method_ == verb::unknown ? std::string_view{ method_string_ } :
        to_string(method_);
}

void header<true>::method_string(const std::string_view& value) NOEXCEPT
{
    method_ = to_verb(value);
    method_string_ = method_ == verb::unknown ? value : std::string_view{};
}

const std::string& header<true>::target() const NOEXCEPT
{
    return target_;
}

void header<true>::target(const std::string_view& value) NOEXCEPT
{
    target_ = value;
}

bool header<true>::need_eof() const NOEXCEPT
{
    return !keep_alive();
}

// header<false>
// ----------------------------------------------------------------------------

header<false>::header() NOEXCEPT
  : header(status::ok, version_1_1)
{
}

header<false>::header(status result, uint32_t version) NOEXCEPT
  : header_base(version), code_(to_value(result)),
    standard_(status_string(result))
{
}

status header<false>::result() const NOEXCEPT
{
    return to_status(code_);
}

void header<false>::result(status value) NOEXCEPT
{
    result(to_value(value));
}

void header<false>::result(uint32_t code) NOEXCEPT
{
    code_ = code;
    standard_ = status_string(result());
}

uint32_t header<false>::result_int() const NOEXCEPT
{
    return code_;
}

std::string header<false>::reason() const NOEXCEPT
{
    return reason_.empty() ? standard_ : reason_;
}

void header<false>::reason(const std::string_view& value) NOEXCEPT
{
    reason_ = value;
}

bool header<false>::bodiless() const NOEXCEPT
{
    return (code_ >= 100u && code_ < 200u) ||
        code_ == to_value(status::no_content) ||
        code_ == to_value(status::not_modified);
}

bool header<false>::need_eof() const NOEXCEPT
{
    if (bodiless() || content_length().has_value() || chunked())
        return !keep_alive();

    return true;
}

// utilities
// ----------------------------------------------------------------------------

bool is_upgrade(const request_header& header) NOEXCEPT
{
    return header.version() >= version_1_1 && header.method() == verb::get &&
        has_token(header[field::connection], "upgrade") &&
        has_token(header[field::upgrade], "websocket");
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
