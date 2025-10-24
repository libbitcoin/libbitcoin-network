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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_ENCODE_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_ENCODE_IPP

#include <cctype>
#include <charconv>
#include <algorithm>
#include <iterator>
#include <regex>

namespace libbitcoin {
namespace network {
namespace json {

TEMPLATE
inline bool CLASS::to_signed(code_t& out, const view_t& token) NOEXCEPT
{
    // JSON-RPC 2.0: Numbers SHOULD NOT contain fractional parts.
    // In other words, numbers can contain fractional parts :[. But we exclude
    // that in the "id" field using this utility. "params" allows JSON numbers.

    // TODO: unit test to ensure empty token always produces false (not zero).
    const auto end = std::next(token.data(), token.size());
    return is_zero(std::from_chars(token.data(), end, out).ec);
}

TEMPLATE
inline bool CLASS::to_number(number_t& out, const view_t& token) NOEXCEPT
{
    static const std::regex number{ R"(-?(0|[1-9]\d*)(\.\d+)?([eE][+-]?\d+)?$)" };

    try
    {
        if (!std::regex_match(token.begin(), token.end(), number))
            return false;
    }
    catch (...)
    {
    }

#if defined(HAVE_APPLE)
    errno = 0;
    char* end = nullptr;
    out = std::strtod(token.data(), &end);
    return (errno != ERANGE) && (end == std::next(token.data(), token.size()));
#else
    const auto end = std::next(token.data(), token.size());
    const auto result = std::from_chars(token.data(), end, out);
    return (is_zero(result.ec) && result.ptr == end);
#endif
}

// Unescape.
// ----------------------------------------------------------------------------
// static

TEMPLATE
string_t CLASS::to_codepoint(const view_t& hi, const view_t& lo) NOEXCEPT
{
    using namespace system;
    data_array<sizeof(char16_t)> bytes{};

    if (!decode_base16(bytes, hi))
        return {};

    const auto high = from_big_endian<char16_t>(bytes);
    if (lo.empty())
        return to_utf8(high);

    if (!decode_base16(bytes, lo))
        return {};

    const auto low = from_big_endian<char16_t>(bytes);
    const auto point =
        add<char32_t>(0x10000,
            add(shift_left(
                bit_and<char32_t>(high, 0x03ff), 10),
                bit_and<char32_t>(low,  0x03ff)));

    return to_utf8(point);
}

// Caller should call unescaped_.clear() [buffer] after assignment.
// The view result of one unescape is not valid after another unescape.
TEMPLATE
bool CLASS::unescape(string_t& buffer, view_t& value) NOEXCEPT
{
    // Shortcircuit if no escapes, preserving first position.
    auto out = value.find('\\');
    if (out == view_t::npos)
        return true;

    // Over-size output string to avoid reallocations.
    const auto value_size = value.size();
    buffer.resize(value_size);

    // Copy unescaped prefix.
    std::copy_n(value.begin(), out, buffer.begin());

    // Copy chunks of unescaped data and process escapes.
    for (auto in = out; in < value_size;)
    {
        // Skip '\' and ensure at least an escape character.
        if (++in == value_size)
            return false;

        // Skip escape character and process.
        switch (value[in++])
        {
            // '/' is unique in that it must be unescaped but may be literal.
            case '/' : buffer[out++] = '/';  break;
            case '"' : buffer[out++] = '"';  break;
            case '\\': buffer[out++] = '\\'; break;
            case 'b' : buffer[out++] = '\b'; break;
            case 'f' : buffer[out++] = '\f'; break;
            case 'n' : buffer[out++] = '\n'; break;
            case 'r' : buffer[out++] = '\r'; break;
            case 't' : buffer[out++] = '\t'; break;
            case 'u' :
            {
                constexpr size_t length = 4;

                view_t hi{};
                if (in + zero + length < value.size())
                {
                    hi = value.substr(in + zero, length);
                    in += zero + length;
                }

                if (hi.empty())
                    return false;

                view_t lo{};
                if (in + two + length < value.size() &&
                    value.substr(in, two) == "\\u")
                {
                    lo = value.substr(in + two, length);
                    in += two + length;
                }

                const auto point = to_codepoint(hi, lo);
                if (point.empty()) return false;
                std::ranges::copy(point, std::next(buffer.begin(), out));
                out += point.size();
                break;
            }
            default:
                return false;
        }

        // Copy > 1 byte chunks of unescaped data.
        if (const auto next = value.find('\\', in); next == view_t::npos)
        {
            // Copy remaining unescaped section (end of value).
            const auto begin = std::next(value.begin(), in);
            const auto end   = value.end();
            const auto to    = std::next(buffer.data(), out);
            std::copy(begin, end, to);
            out += value_size - in;
            break;
        }
        else if (next > in)
        {
            // Copy unescaped section before escape (inside value).
            const auto begin = std::next(value.begin(), in);
            const auto end   = std::next(value.begin(), next);
            const auto to    = std::next(buffer.data(), out);
            std::copy(begin, end, to);
            out += next - in;
            in = next;
        }
    }

    buffer.resize(out);
    value = view_t{ buffer };
    return true;
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
