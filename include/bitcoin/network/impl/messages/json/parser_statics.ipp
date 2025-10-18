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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_STATICS_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_STATICS_IPP

namespace libbitcoin {
namespace network {
namespace json {
    
// protected/static

TEMPLATE
inline json::error_code CLASS::parse_error() NOEXCEPT
{
    namespace errc = boost::system::errc;
    return errc::make_error_code(errc::invalid_argument);
}

TEMPLATE
bool CLASS::is_null(const id_t& id) NOEXCEPT
{
    return std::holds_alternative<null_t>(id);
}

TEMPLATE
bool CLASS::is_whitespace(char c) NOEXCEPT
{
    return (c == ' ' || c == '\n' || c == '\r' || c == '\t');
}

TEMPLATE
inline bool CLASS::to_number(int64_t& out, view token) NOEXCEPT
{
    const auto end = std::next(token.data(), token.size());
    return is_zero(std::from_chars(token.data(), end, out).ec);
}

TEMPLATE
inline id_t CLASS::to_id(view token) NOEXCEPT
{
    int64_t out{};
    if (to_number(out, token))
    {
        return out;
    }
    else if (token == "null")
    {
        return null_t{};
    }
    else
    {
        return string_t{ token };
    }
}

TEMPLATE
inline bool CLASS::increment(size_t& depth, state& status) NOEXCEPT
{
    if (is_zero(++depth))
    {
        status = state::error_state;
        return false;
    }

    return true;
}

TEMPLATE
inline bool CLASS::decrement(size_t& depth, state& status) NOEXCEPT
{
    if (is_zero(depth--))
    {
        status = state::error_state;
        return false;
    }

    return true;
}

TEMPLATE
inline void CLASS::consume(view& token, const char_iterator& at) NOEXCEPT
{
    // Token consumes character *at by incrementing its view over at's buffer.
    if (token.empty())
        token = { std::to_address(at), one };
    else
        token = { token.data(), add1(token.size()) };
}

TEMPLATE
inline size_t CLASS::distance(const char_iterator& from,
    const char_iterator& to) NOEXCEPT
{
    using namespace system;
    return possible_narrow_and_sign_cast<size_t>(std::distance(from, to));
}
 
} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
