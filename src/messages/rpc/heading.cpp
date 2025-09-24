/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/messages/rpc/heading.hpp>

#include <algorithm>
#include <optional>
#include <ranges>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// local utils
// ----------------------------------------------------------------------------

constexpr auto backslash = '\\';
constexpr auto double_quote = '"';
constexpr auto left_parenthesis = '(';
constexpr auto right_parenthesis = ')';
constexpr char token_special[]
{
    0x21, // ascii 33, '!'
    0x23, // ascii 35, '#'
    0x24, // ascii 36, '$'
    0x25, // ascii 37, '%'
    0x26, // ascii 38, '&'
    0x27, // ascii 39, '''
    0x2a, // ascii 42, '*'
    0x2b, // ascii 43, '+'
    0x2d, // ascii 45, '-'
    0x2e, // ascii 46, '.'
    0x5e, // ascii 94, '^'
    0x5f, // ascii 95, '_'
    0x60, // ascii 96, '`'
    0x7c, // ascii 124, '|'
    0x7e  // ascii 126, '~'
};

constexpr bool is_token_special(char point) NOEXCEPT
{
    return std::ranges::any_of(token_special, [=](auto special) NOEXCEPT
    {
        return special == point;
    });
}

constexpr bool is_token_character(char point) NOEXCEPT
{
    return is_ascii_alpha(point)
        || is_ascii_number(point)
        || is_token_special(point);
}

constexpr bool is_token(std::string_view text) NOEXCEPT
{
    return std::ranges::all_of(text, is_token_character);
}

constexpr bool is_http_whitespace(char point) NOEXCEPT
{
    // ' ', '\t'
    return point == 0x20 || point == 0x09;
};

constexpr bool is_obsolete_text(char /*point*/) NOEXCEPT
{
    // 8th bit
    ////return 0x80 <= point && point <= 0xff;
    return false;
};

// A sender SHOULD NOT generate a quoted-pair in a comment except where
// necessary to quote parentheses ["(" and ")"] and backslash octets occurring
// within that comment.
constexpr bool is_escapable_comment_text(char point) NOEXCEPT
{
    return point == left_parenthesis || point == right_parenthesis ||
        point == backslash;
}

// Combines with is_http_whitespace(point).
constexpr bool is_comment_text(char point) NOEXCEPT
{
    return
        (point >= 0x21 && point <= 0x27) ||     // '!' - '''
        (point >= 0x2a && point <= 0x5b) ||     // '*' - '['
        (point >= 0x5d && point <= 0x7e) ||     // ']' - '~'
        is_obsolete_text(point);
};

// This is redundant with inclusion of only whitespace and (visible or quoted).
constexpr bool is_control_text(char point) NOEXCEPT
{
    // CTL except \t
    return (point <= 0x1f && point != 0x09) || point == 0x7f;
};

constexpr bool is_visible_text(char point) NOEXCEPT
{
    // '!' - '~'
    return 0x21 <= point && point <= 0x7e ||
        is_obsolete_text(point);
};

// A sender SHOULD NOT generate a quoted-pair in a quoted-string except where
// necessary to quote DQUOTE and backslash octets occurring within that string.
constexpr bool is_escapable_quoted_text(char point) NOEXCEPT
{
    return point == double_quote || point == backslash;
}

constexpr bool is_quoted_text(char point) NOEXCEPT
{
    return point == 0x21 ||                     // '!'
        (point >= 0x23 && point <= 0x5b) ||     // '#' - '['
        (point >= 0x5d && point <= 0x7e) ||     // ']' - '~'
        is_obsolete_text(point);
};

constexpr bool is_http_quoted(std::string_view text) NOEXCEPT
{
    return (text.size() > one) && text.front() == double_quote &&
        text.back() == double_quote;
}

// public
// ----------------------------------------------------------------------------

const std::string heading::tab{ "\t" };
const std::string heading::space{ " " };
const std::string heading::separator{ ":" };
const std::string heading::separators{ ": " };
const std::string heading::crlfx2{ "\r\n\r\n" };
const std::string heading::crlf{ "\r\n" };
const string_list heading::whitespace{ space, tab };

bool heading::validate_unquoted_value(std::string_view value) NOEXCEPT
{
    BC_ASSERT(!is_http_quoted(value));

    if (std::ranges::all_of(value, is_http_whitespace))
        return value.empty();

    return std::ranges::all_of(value, [](char point) NOEXCEPT
    {
        return (is_visible_text(point) || is_http_whitespace(point));
    });
}

heading::string_t heading::unescape_quoted_value(
    std::string_view value) NOEXCEPT
{
    BC_ASSERT(is_http_quoted(value));

    std::string out{};
    out.reserve(value.size());
    auto paired = false;
    for (auto index = one; index < sub1(value.size()); ++index)
    {
        const char point = value.at(index);

        if (paired)
        {
            if (is_http_whitespace(point) || is_escapable_quoted_text(point))
            {
                out += point;
                paired = false;
            }
            else
            {
                return std::nullopt;
            }
        }
        else if (point == backslash)
        {
            paired = true;
        }
        else if (is_http_whitespace(point) || is_quoted_text(point))
        {
            out += point;
        }
        else
        {
            return std::nullopt;
        }
    }

    if (paired)
        return std::nullopt;
    
    return out;
}

heading::string_t heading::to_field_name(const std::string& name) NOEXCEPT
{
    if (!is_token(name))
        return std::nullopt;

    return ascii_to_lower(name);
}

heading::string_t heading::to_field_value(std::string&& value) NOEXCEPT
{
    const auto out = trim_copy(std::move(value), heading::whitespace);
    if (out.empty())
        return out;

    if (is_http_quoted(out))
        return unescape_quoted_value(out);

    if (!validate_unquoted_value(out))
        return std::nullopt;

    return out;
}

// public
// ----------------------------------------------------------------------------

size_t heading::fields_size(const fields& headers) NOEXCEPT
{
    return std::accumulate(headers.begin(), headers.end(), zero,
        [](size_t sum, const auto& pair) NOEXCEPT
        {
            return sum +
                pair.first.size() + heading::separators.size() +
                pair.second.size() + heading::crlf.size();
        });
};

// Ordered multimap ensures consistent output order. However original order is
// required for certain fields, so adjust as necessary if they are implemented.
heading::fields heading::to_fields(reader& source) NOEXCEPT
{
    fields out{};

    // Read until empty/fail or line starts with first line character.
    while (!source.is_exhausted() && (source.peek_byte() != crlf.front()))
    {
        auto token = to_field_name(source.read_line(separator));
        auto value = to_field_value(source.read_line());

        if (!token || !value)
            source.invalidate();
        else
            out.emplace(std::move(*token), std::move(*value));
    }

    // Headers end with empty line.
    if (!source.read_line().empty())
        source.invalidate();

    if (!source)
        return {};

    // name:value pairs not validated with respect to specific field types.
    return out;
}

void heading::from_fields(const fields& fields, writer& sink) NOEXCEPT
{
    std::ranges::for_each(fields, [&sink](const auto& field) NOEXCEPT
    {
        sink.write_line(field.first, separators);
        sink.write_line(field.second);
    });

    // Fields end with empty line.
    sink.write_line();
}

BC_POP_WARNING()

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
