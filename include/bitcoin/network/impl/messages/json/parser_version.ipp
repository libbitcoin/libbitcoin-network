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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_VERSION_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_VERSION_IPP

namespace libbitcoin {
namespace network {
namespace json {

// protected

TEMPLATE
inline json::version CLASS::to_version(const view_t& token) const NOEXCEPT
{
    if (allow_version1() && (token == "1.0" || token.empty()))
        return version::v1;

    if (allow_version2() && token == "2.0")
        return version::v2;

    // assign_version() expects this in case of invalid value.
    return version::invalid;
}

TEMPLATE
inline bool CLASS::allow_version1() const NOEXCEPT
{
    return require == version::any || require == version::v1;
}

TEMPLATE
inline bool CLASS::allow_version2() const NOEXCEPT
{
    return require == version::any || require == version::v2;
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
