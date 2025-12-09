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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_WS_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_WS_HPP

#include <memory>
#include <optional>
#include <bitcoin/network/channels/channel_http.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

/// Abstract base websocket tcp/ip channel, on base http channel.
class BCT_API channel_ws
  : public channel_http, protected tracker<channel_ws>
{
public:
    typedef std::shared_ptr<channel_ws> ptr;
    using options_t = settings_t::websocket_server;

protected:
    inline channel_ws(const logger& log, const socket::ptr& socket,
        uint64_t identifier, const settings_t& settings,
        const options_t& options) NOEXCEPT
      : channel_http(log, socket, identifier, settings, options),
        tracker<channel_ws>(log)
    {
    }

    /// Reads are never buffered, restart the reader.
    void read_request() NOEXCEPT override;

    /// Pre-upgrade http read.
    void handle_read_request(const code& ec, size_t bytes,
        const http::request_cptr& request) NOEXCEPT override;

    /// Post-upgrade websocket read.
    virtual void handle_read_websocket(const code& ec, size_t bytes) NOEXCEPT;

    /// Dispatch websocket buffer via derived handlers (override to handle).
    /// Override to handle dispatch, must invoke read_request() on complete.
    virtual void dispatch_websocket(const http::flat_buffer& buffer,
        size_t bytes) NOEXCEPT;

private:
    // This is protected by strand.
    bool upgraded_{ false };
};

} // namespace network
} // namespace libbitcoin

#endif
