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
#include <bitcoin/network/messages/rpc/enums/method.hpp>

#include <unordered_map>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// datatracker.ietf.org/doc/html/rfc9110#name-overview
// PATCH extension: datatracker.ietf.org/doc/html/rfc5789 

method to_method(const std::string& value) NOEXCEPT
{
    static const std::unordered_map<std::string, method> map
    {
        { "GET", method::get },
        { "POST", method::post },
        { "PUT", method::put },
        { "PATCH", method::patch },
        { "DELETE", method::delete_ },
        { "HEAD", method::head },
        { "OPTIONS", method::options },
        { "TRACE", method::trace },
        { "CONNECT", method::connect },
        { "UNDEFINED", method::undefined }
    };

    const auto found = map.find(value);
    return found == map.end() ? method::undefined : found->second;
}

const std::string& from_method(method value) NOEXCEPT
{
    static const std::unordered_map<method, std::string> map
    {
        { method::get, "GET" },
        { method::post, "POST" },
        { method::put, "PUT" },
        { method::patch, "PATCH" },
        { method::delete_, "DELETE" },
        { method::head, "HEAD" },
        { method::options, "OPTIONS" },
        { method::trace, "TRACE" },
        { method::connect, "CONNECT" },
        { method::undefined, "UNDEFINED" }
    };

    return map.at(value);
}

BC_POP_WARNING()

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
