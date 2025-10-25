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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_SERIALIZER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_SERIALIZER_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

struct BCT_API serializer
{
    // Serializes the request_t to a compact JSON string (for testing).
    // Handles flat blob strings in params structures as literal JSON.
    static string_t write(const request_t& request) NOEXCEPT;

    // Serializes the response_t to a compact JSON string.
    // Handles flat blob strings in result structure as literal JSON.
    static string_t write(const response_t& response) NOEXCEPT;
};

////DECLARE_JSON_VALUE_CONVERTORS(request_t);
////DECLARE_JSON_VALUE_CONVERTORS(response_t);

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
