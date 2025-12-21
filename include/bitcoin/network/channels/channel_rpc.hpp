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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_RPC_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_RPC_HPP

#include <memory>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/net.hpp>

namespace libbitcoin {
namespace network {

/// Read rpc-request and send rpc-response.
class BCT_API channel_rpc
  : public channel
{
public:
    typedef std::shared_ptr<channel_rpc> ptr;

    inline channel_rpc(const logger& log, const socket::ptr& socket,
        uint64_t identifier, const settings_t& settings,
        const options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options)
    {
    }

    /// Resume reading from the socket (requires strand).
    void resume() NOEXCEPT override;

    /// Must call after successful message handling if no stop.
    virtual void receive() NOEXCEPT;

    /// Serialize and write rpc response to client (requires strand).
    /// Completion handler is always invoked on the channel strand.
    virtual void send() NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
