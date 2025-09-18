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
#include <bitcoin/network/messages/rpc/enums/version.hpp>

#include <unordered_map>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

version to_version(const std::string& value) NOEXCEPT
{
    static const std::unordered_map<std::string, version> map
    {
        { "HTTP/0.9", version::http_0_9 },
        { "HTTP/1.0", version::http_1_0 },
        { "HTTP/1.1", version::http_1_1 },
        { "UNDEFINED/0.0", version::undefined }
    };

    const auto found = map.find(value);
    return found == map.end() ? version::undefined : found->second;
}

const std::string& from_version(version value) NOEXCEPT
{
    static const std::unordered_map<version, std::string> map
    {
        { version::http_0_9, "HTTP/0.9" },
        { version::http_1_0, "HTTP/1.0" },
        { version::http_1_1,"HTTP/1.1" },
        { version::undefined, "UNDEFINED/0.0" }
    };

    return map.at(value);
}

BC_POP_WARNING()

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
