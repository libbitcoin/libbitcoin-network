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
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

using namespace system;

const std::string heading::space{ " " };
const std::string heading::separator{ ":" };
const std::string heading::separators{ ": " };
const std::string heading::line{ "\r\n" };
const std::string heading::terminal{ "\r\n\r\n" };

const char token_special_chars[]
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

constexpr bool is_token_special(char character) NOEXCEPT
{
    return std::any_of(std::begin(token_special_chars),
        std::end(token_special_chars), [=](auto special) NOEXCEPT
        {
            return special = character;
        });
}

constexpr bool is_token_character(char character) NOEXCEPT
{
    return is_ascii_alpha(character)
        || is_ascii_number(character)
        || is_token_special(character);
}

constexpr bool is_token(const std::string& text) NOEXCEPT
{
    return std::all_of(text.begin(), text.end(), is_token_character);
}

size_t heading::fields_size(const fields& headers) NOEXCEPT
{
    return std::accumulate(headers.begin(), headers.end(), zero,
        [](size_t sum, const auto& pair) NOEXCEPT
        {
            return sum +
                pair.first.size() + heading::separators.size() +
                pair.second.size() + heading::line.size();
        });
};

heading::fields heading::to_fields(reader& source) NOEXCEPT
{
    fields out{};

    // Read until empty/fail or line starts with first line character.
    while (!source.is_exhausted() && (source.peek_byte() != line.front()))
    {
        const auto token = source.read_line(separator);
        if (!is_token(token))
            return {};

        out.emplace(ascii_to_lower(token), trim_copy(source.read_line()));
    }

    // Headers end with empty line.
    if (!source.read_line().empty())
        source.invalidate();

    if (!source)
        return {};

    // name:value pairs are not validated at this point.
    return out;
}

void heading::from_fields(const fields& fields, writer& sink) NOEXCEPT
{
    // Write all fields.
    std::for_each(fields.begin(), fields.end(),
        [&sink](const auto& header) NOEXCEPT
        {
            sink.write_line(header.first, separators);
            sink.write_line(header.second);
        });

    // Fields end with empty line.
    sink.write_line();
}

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
