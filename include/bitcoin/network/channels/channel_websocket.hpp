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
#include <optional>
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

    /// Subscribe to WS messages post-upgrade (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Message>
    inline void subscribe(auto&& ) NOEXCEPT
    {
        BC_ASSERT(stranded());
        ////using message_handler = distributor_ws::handler<Message>;
        ////ws_distributor_.subscribe(std::forward<message_handler>(handler));
    }

    /// Serialize and write WS message to peer (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <class Message>
    inline void send(Message&& message, result_handler&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());
        BC_ASSERT(websocket());
        using namespace std::placeholders;

        // TODO: Serialize ws message.
        const auto ptr = system::move_shared(std::forward<Message>(message));
        count_handler complete = std::bind(&channel_websocket::handle_send,
            shared_from_base<channel_websocket>(), _1, _2, ptr,
            std::move(handler));

        if (!ptr)
        {
            handler(error::bad_alloc);
            return;
        }

        // TODO: serialize websocket message to send.
        // TODO: websocket is full duplex, so writes must be queued.
        ws_write({}, std::move(complete));
    }

    inline channel_websocket(const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier={},
        const options_t& options={}) NOEXCEPT
      : channel_http(log, socket, settings, identifier, options),
        ////distributor_(socket->strand()),
        tracker<channel_websocket>(log)
    {
    }

    /// Half-duplex http until upgraded to full-duplex websockets.
    void read_request() NOEXCEPT override;

protected:
    void send_websocket_accept(const http::request& request) NOEXCEPT;
    void handle_read_request(const code& ec, size_t bytes,
        const http::request_cptr& request) NOEXCEPT override;
    virtual void handle_read_websocket(const code& ec, size_t bytes) NOEXCEPT;

    void handle_upgrade(const http::request& request) NOEXCEPT;
    void handle_upgrade_complete(const code& ec) NOEXCEPT;

private:
    inline void handle_send(const code& ec, size_t bytes, const auto&,
        const result_handler& handler) NOEXCEPT
    {
        if (ec) stop(ec);
        handler(ec);
    }

    // These are protected by strand.
    ////distributor_rest distributor_;
    bool upgraded_{ false };
};

} // namespace network
} // namespace libbitcoin

#endif
