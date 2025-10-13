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
#ifndef LIBBITCOIN_NETWORK_NET_CHANNEL_WEBSOCKET_HPP
#define LIBBITCOIN_NETWORK_NET_CHANNEL_WEBSOCKET_HPP

#include <memory>
#include <bitcoin/network/net/channel.hpp>

namespace libbitcoin {
namespace network {

/// Full websocket peer-to-peer tcp/ip channel.
class BCT_API channel_websocket
  : public channel, protected tracker<channel_websocket>
{
public:
    typedef std::shared_ptr<channel_websocket> ptr;
};

} // namespace network
} // namespace libbitcoin

#endif
