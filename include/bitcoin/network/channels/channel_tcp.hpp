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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_TCP_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_TCP_HPP

#include <memory>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

// TODO: implement.
class BCT_API channel_tcp
  : public channel, protected tracker<channel_tcp>
{
public:
    typedef std::shared_ptr<channel_tcp> ptr;
    using options_t = settings::tcp_server;

    channel_tcp(const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero,
        const options_t& options={}) NOEXCEPT
      : channel(log, socket, settings, identifier, options.timeout()),
        tracker<channel_tcp>(log)
    {
    }
};

} // namespace network
} // namespace libbitcoin

#endif
