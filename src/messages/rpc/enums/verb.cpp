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
#include <bitcoin/network/messages/rpc/enums/verb.hpp>

#include <unordered_map>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

verb to_verb(const std::string& value) NOEXCEPT
{
    static const std::unordered_map<std::string, verb> map
    {
        { "GET", verb::get },
        { "POST", verb::post },
        { "PUT", verb::put },
        { "PATCH", verb::patch },
        { "DELETE", verb::delete_ },
        { "HEAD", verb::head },
        { "OPTIONS", verb::options },
        { "TRACE", verb::trace },
        { "CONNECT", verb::connect },
        { "UNDEFINED", verb::undefined }
    };

    const auto found = map.find(value);
    return found == map.end() ? verb::undefined : found->second;
}

const std::string& from_verb(verb value) NOEXCEPT
{
    static const std::unordered_map<verb, std::string> map
    {
        { verb::get, "GET" },
        { verb::post, "POST" },
        { verb::put, "PUT" },
        { verb::patch, "PATCH" },
        { verb::delete_, "DELETE" },
        { verb::head, "HEAD" },
        { verb::options, "OPTIONS" },
        { verb::trace, "TRACE" },
        { verb::connect, "CONNECT" },
        { verb::undefined, "UNDEFINED" }
    };

    return map.at(value);
}

BC_POP_WARNING()

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
