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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_WEBSOCKET_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_WEBSOCKET_HPP

#include <memory>
#include <bitcoin/network/channels/channel_http.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Websocket tcp/ip channel, uses channel_http for upgrade/multiplex.
class BCT_API channel_websocket
  : public channel_http, protected tracker<channel_websocket>
{
public:
    typedef std::shared_ptr<channel_websocket> ptr;
    using options_t = settings::websocket_server;

    inline channel_websocket(const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero,
        const options_t& options={}) NOEXCEPT
      : channel_http(log, socket, settings, identifier, options),
        tracker<channel_websocket>(log)
    {
    }
};

} // namespace network
} // namespace libbitcoin

#endif
