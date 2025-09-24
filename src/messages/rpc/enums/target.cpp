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
#include <bitcoin/network/messages/rpc/enums/target.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/enums/method.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

target to_target(const std::string& value, method method) NOEXCEPT
{
    // Only for OPTIONS method.
    if (method == method::options && value == "*")
        return target::asterisk;

    // TODO: use URI parser for rpc::target::authority (only for CONNECT).
    else if (method == method::connect)
        return target::undefined;

    // TODO: validate path (any method).
    else if (value.starts_with("/"))
        return target::origin;

    // TODO: use URI parser for unescape and full validation (any method).
    else if (value.starts_with("http://") || value.starts_with("https://"))
        return target::absolute;

    return target::undefined;
}

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
