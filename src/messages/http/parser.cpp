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
#include <bitcoin/network/messages/http/parser.hpp>

#include <algorithm>
#include <charconv>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/fields.hpp>
#include <bitcoin/network/messages/http/header.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace system;
using namespace network::error;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Each parse function consumes from the front of its text, returning the
// number of characters consumed, or zero with need_more if the text ends
// before the element does. Validation proceeds character by character, so an
// invalid element is reported once its first invalid character is present.

// Character classes (rfc9110).
// ----------------------------------------------------------------------------

constexpr bool is_digit(char character) NOEXCEPT
{
    return character >= '0' && character <= '9';
}

// tchar = "!" / "#" / "$" / "%" / "&" / "'" / "*" / "+" / "-" / "." /
//         "^" / "_" / "`" / "|" / "~" / DIGIT / ALPHA
constexpr bool is_tchar(char character) NOEXCEPT
{
    constexpr std::string_view others{ "!#$%&'*+-.^_`|~" };
    return is_digit(character) ||
        (character >= 'a' && character <= 'z') ||
        (character >= 'A' && character <= 'Z') ||
        others.find(character) != std::string_view::npos;
}

// Visible characters and obs-text.
constexpr bool is_target_char(char character) NOEXCEPT
{
    const auto byte = static_cast<uint8_t>(character);
    return byte > 0x20 && byte != 0x7f;
}

// Control characters other than horizontal tab.
constexpr bool is_control(char character) NOEXCEPT
{
    const auto byte = static_cast<uint8_t>(character);
    return (byte < 0x20 && byte != '\t') || byte == 0x7f;
}

constexpr bool is_whitespace(char character) NOEXCEPT
{
    return character == ' ' || character == '\t';
}

constexpr int8_t to_nibble(char character) NOEXCEPT
{
    if (is_digit(character))
        return character - '0';

    if (character >= 'a' && character <= 'f')
        return 10 + (character - 'a');

    if (character >= 'A' && character <= 'F')
        return 10 + (character - 'A');

    return -1;
}

static void set_need_more(boost_code& ec) NOEXCEPT
{
    ec = to_http_code(http_error_t::need_more);
}

// Find the line end (CRLF) of text without control characters, setting text
// to the index of the CR and end to the index following the LF. False with ec
// clear on a control character, false with ec set if the line is incomplete or
// a CR does not precede an LF.
static bool find_line(size_t& text, size_t& end, const std::string_view& in,
    size_t start, boost_code& ec) NOEXCEPT
{
    for (auto index = start; index < in.size(); ++index)
    {
        const auto character = in[index];
        if (!is_control(character))
            continue;

        if (character != '\r')
        {
            ec.clear();
            return false;
        }

        if (add1(index) == in.size())
        {
            set_need_more(ec);
            return false;
        }

        if (in[add1(index)] != '\n')
        {
            ec = to_http_code(http_error_t::bad_line_ending);
            return false;
        }

        text = index;
        end = index + two;
        ec.clear();
        return true;
    }

    set_need_more(ec);
    return false;
}

static size_t trim_back(const std::string_view& in, size_t first,
    size_t last) NOEXCEPT
{
    while (last > first && is_whitespace(in[sub1(last)]))
        --last;

    return last;
}

// HTTP-version = "HTTP/" DIGIT "." DIGIT, limited to 1.0 and 1.1.
static bool parse_version(uint32_t& version, const std::string_view& in,
    size_t& index, boost_code& ec) NOEXCEPT
{
    constexpr auto size = 8u;
    if (in.size() - index < size)
    {
        set_need_more(ec);
        return false;
    }

    const auto text = in.substr(index, size);
    if (!text.starts_with("HTTP/") || !is_digit(text[5]) || text[6] != '.' ||
        !is_digit(text[7]))
    {
        ec = to_http_code(http_error_t::bad_version);
        return false;
    }

    version = (text[5] - '0') * 10u + (text[7] - '0');
    if (version < version_1_0 || version > version_1_1)
    {
        ec = to_http_code(http_error_t::bad_version);
        return false;
    }

    index += size;
    return true;
}

// Start lines.
// ----------------------------------------------------------------------------

// request-line = method SP request-target SP HTTP-version CRLF
size_t parse_start_line(request_header& out, const std::string_view& in,
    boost_code& ec) NOEXCEPT
{
    size_t index{};
    while (index < in.size() && is_tchar(in[index]))
        ++index;

    if (index == in.size())
    {
        set_need_more(ec);
        return {};
    }

    if (in[index] != ' ' || is_zero(index))
    {
        ec = to_http_code(http_error_t::bad_method);
        return {};
    }

    const auto method = in.substr(zero, index++);
    const auto first = index;
    while (index < in.size() && is_target_char(in[index]))
        ++index;

    if (index == in.size())
    {
        set_need_more(ec);
        return {};
    }

    if (in[index] != ' ' || index == first)
    {
        ec = to_http_code(http_error_t::bad_target);
        return {};
    }

    const auto target = in.substr(first, index++ - first);
    uint32_t version{};
    if (!parse_version(version, in, index, ec))
        return {};

    if (in.size() - index < two)
    {
        set_need_more(ec);
        return {};
    }

    if (in[index] != '\r' || in[add1(index)] != '\n')
    {
        ec = to_http_code(http_error_t::bad_version);
        return {};
    }

    out.method_string(method);
    out.target(target);
    out.version(version);
    ec.clear();
    return index + two;
}

// status-line = HTTP-version SP status-code SP reason-phrase CRLF
size_t parse_start_line(response_header& out, const std::string_view& in,
    boost_code& ec) NOEXCEPT
{
    size_t index{};
    uint32_t version{};
    if (!parse_version(version, in, index, ec))
        return {};

    if (index == in.size())
    {
        set_need_more(ec);
        return {};
    }

    if (in[index++] != ' ')
    {
        ec = to_http_code(http_error_t::bad_version);
        return {};
    }

    constexpr auto status_size = 4u;
    if (in.size() - index < status_size)
    {
        set_need_more(ec);
        return {};
    }

    if (!is_digit(in[index]) || !is_digit(in[add1(index)]) ||
        !is_digit(in[index + two]) || in[index + 3u] != ' ')
    {
        ec = to_http_code(http_error_t::bad_status);
        return {};
    }

    const auto code = (in[index] - '0') * 100u + (in[add1(index)] - '0') *
        10u + (in[index + two] - '0');

    index += status_size;
    size_t text{};
    size_t end{};
    if (!find_line(text, end, in, index, ec))
    {
        if (!ec)
            ec = to_http_code(http_error_t::bad_reason);

        return {};
    }

    out.version(version);
    out.result(code);
    out.reason(in.substr(index, text - index));
    ec.clear();
    return end;
}

// Fields.
// ----------------------------------------------------------------------------

// #token = [ ( "," / token ) *( OWS "," [ OWS token ] ) ]
// The leading tokens of the list, up to the first invalid element (valid is
// false if there is one).
static string_list token_list(const std::string_view& value,
    bool& valid) NOEXCEPT
{
    string_list out{};
    size_t index{};
    bool need_comma{};
    valid = true;
    while (true)
    {
        while (index < value.size() && is_whitespace(value[index]))
            ++index;

        if (index == value.size())
            return out;

        const auto character = value[index];
        if (is_tchar(character))
        {
            if (need_comma)
            {
                valid = false;
                return out;
            }

            const auto first = index;
            while (index < value.size() && is_tchar(value[index]))
                ++index;

            out.emplace_back(value.substr(first, index - first));
            need_comma = true;
            continue;
        }

        if (character != ',')
        {
            valid = false;
            return out;
        }

        need_comma = false;
        ++index;
    }
}

// Connection must be a valid token list.
// Content-Length may be a list of equal values, and conflicts with chunked.
// Transfer-Encoding conflicts with Content-Length and may not follow chunked.
// Chunked framing requires that the first chunked coding be the last coding.
static void frame_field(framing& frame, field name,
    const std::string_view& value, boost_code& ec) NOEXCEPT
{
    ec.clear();
    if (name == field::connection)
    {
        bool valid{};
        token_list(value, valid);
        if (!valid)
            ec = to_http_code(http_error_t::bad_value);

        return;
    }

    if (name == field::content_length)
    {
        bool valid{};
        token_list(value, valid);
        if (frame.chunked || frame.encoded || !valid)
        {
            ec = to_http_code(http_error_t::bad_content_length);
            return;
        }

        const auto items = split(value, { "," }, http_whitespace);
        for (const auto& item: items)
        {
            uint64_t number{};
            const auto last = std::next(item.data(), item.size());
            const auto [next, error] = std::from_chars(item.data(), last,
                number);

            if (item.empty() || !is_digit(item.front()) ||
                error != std::errc{} || next != last)
            {
                ec = to_http_code(http_error_t::bad_content_length);
                return;
            }

            if (frame.length.has_value() && number != frame.length.value())
            {
                ec = to_http_code(http_error_t::multiple_content_length);
                return;
            }

            frame.length = number;
        }

        return;
    }

    if (name == field::transfer_encoding)
    {
        if (frame.chunked || frame.encoded || frame.length.has_value())
        {
            ec = to_http_code(http_error_t::bad_transfer_encoding);
            return;
        }

        bool valid{};
        const auto codings = token_list(value, valid);
        if (!valid || codings.empty())
        {
            ec = to_http_code(http_error_t::bad_transfer_encoding);
            return;
        }

        const auto it = std::ranges::find_if(codings,
            [](const auto& coding) NOEXCEPT
            {
                return iequals(coding, "chunked");
            });

        frame.chunked = it != codings.end() && std::next(it) == codings.end();
        frame.encoded = !frame.chunked;
    }
}

// header-field = field-name ":" OWS field-value OWS, where folded lines
// (obs-fold) are joined by a space.
size_t parse_field(fields* out, framing& frame, bool& end,
    const std::string_view& in, boost_code& ec) NOEXCEPT
{
    end = false;
    if (in.size() < two)
    {
        set_need_more(ec);
        return {};
    }

    if (in.front() == '\r')
    {
        if (in[one] != '\n')
        {
            ec = to_http_code(http_error_t::bad_line_ending);
            return {};
        }

        end = true;
        ec.clear();
        return two;
    }

    size_t index{};
    while (true)
    {
        if (index == in.size())
        {
            set_need_more(ec);
            return {};
        }

        if (in[index] == ':')
            break;

        if (!is_tchar(in[index]))
        {
            ec = to_http_code(http_error_t::bad_field);
            return {};
        }

        ++index;
    }

    if (is_zero(index))
    {
        ec = to_http_code(http_error_t::bad_field);
        return {};
    }

    const auto name = in.substr(zero, index++);
    std::string value{};
    while (true)
    {
        while (true)
        {
            if (index == in.size())
            {
                set_need_more(ec);
                return {};
            }

            if (!is_whitespace(in[index]))
                break;

            ++index;
        }

        size_t text{};
        size_t line{};
        if (!find_line(text, line, in, index, ec))
        {
            if (!ec)
                ec = to_http_code(http_error_t::bad_value);

            return {};
        }

        // The character following the line end determines folding.
        if (line == in.size())
        {
            set_need_more(ec);
            return {};
        }

        const auto last = trim_back(in, index, text);
        if (last != index)
        {
            if (!value.empty())
                value += ' ';

            value.append(in.substr(index, last - index));
        }

        index = line;
        if (!is_whitespace(in[index]))
            break;

        ++index;
    }

    // Proxy-Connection (not enumerated) is validated as Connection.
    const auto id = iequals(name, "Proxy-Connection") ? field::connection :
        to_field(name);

    frame_field(frame, id, value, ec);
    if (ec)
        return {};

    if (!is_null(out))
        out->insert(name, value);

    return index;
}

// Chunks.
// ----------------------------------------------------------------------------

size_t parse_chunk_size(uint64_t& size, const std::string_view& line,
    boost_code& ec) NOEXCEPT
{
    size = zero;
    size_t index{};
    for (; index < line.size(); ++index)
    {
        const auto nibble = to_nibble(line[index]);
        if (is_negative(nibble))
            break;

        if (size > (max_uint64 >> 4))
        {
            ec = to_http_code(http_error_t::bad_chunk);
            return {};
        }

        size = (size << 4) | to_unsigned(nibble);
    }

    if (is_zero(index))
    {
        ec = to_http_code(http_error_t::bad_chunk);
        return {};
    }

    ec.clear();
    return index;
}

// chunk-ext = *( BWS ";" BWS chunk-ext-name [ BWS "=" BWS chunk-ext-val ] ),
// chunk-ext-val = token / quoted-string. Valid if they extend to the line end.
bool valid_chunk_extensions(const std::string_view& line,
    size_t index) NOEXCEPT
{
    // The line end (CR) follows the line.
    const auto at = [&](size_t position) NOEXCEPT
    {
        return position < line.size() ? line[position] : '\r';
    };

    const auto skip = [&]() NOEXCEPT
    {
        while (is_whitespace(at(index)))
            ++index;
    };

    while (true)
    {
        if (!is_whitespace(at(index)) && at(index) != ';')
            return index == line.size();

        skip();
        if (at(index) != ';')
            return false;

        while (true)
        {
            ++index;
            skip();
            if (!is_tchar(at(index)))
                return false;

            while (is_tchar(at(index)))
                ++index;

            const auto spaced = is_whitespace(at(index));
            skip();
            if (at(index) == ';')
                continue;

            if (at(index) != '=')
            {
                if (spaced)
                    return false;

                break;
            }

            ++index;
            skip();
            if (at(index) != '"')
            {
                if (!is_tchar(at(index)))
                    return false;

                while (is_tchar(at(index)))
                    ++index;
            }
            else
            {
                while (true)
                {
                    ++index;
                    if (index >= line.size())
                        return false;

                    if (line[index] == '"')
                        break;

                    if (line[index] == '\\')
                        ++index;
                }

                ++index;
            }

            break;
        }
    }
}

// The line end (CRLF) of a chunk line, which may contain any characters but
// a CR that does not precede an LF.
size_t find_chunk_line(size_t& text, const std::string_view& in,
    boost_code& ec) NOEXCEPT
{
    for (size_t index{}; index < in.size(); ++index)
    {
        if (in[index] != '\r')
            continue;

        if (add1(index) == in.size())
            break;

        if (in[add1(index)] != '\n')
        {
            ec = to_http_code(http_error_t::bad_line_ending);
            return {};
        }

        text = index;
        ec.clear();
        return index + two;
    }

    set_need_more(ec);
    return {};
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
