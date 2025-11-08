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
#include <bitcoin/network/channels/channel_ws.hpp>

#include <optional>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/http.hpp>

namespace libbitcoin {
namespace network {

#define CLASS channel_ws

using namespace system;
using namespace network::http;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// public/virtual
// ----------------------------------------------------------------------------

void channel_ws::read_request() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped() || paused() || !upgraded_)
    {
        channel_http::read_request();
        return;
    }

    ws_read(request_buffer(),
        std::bind(&channel_ws::handle_read_websocket,
            shared_from_base<channel_ws>(), _1, _2));
}

// upgraded
// ----------------------------------------------------------------------------

void channel_ws::handle_read_websocket(const code& ec,
    size_t) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        LOGQ("Websocket read abort [" << authority() << "]");
        return;
    }

    if (ec)
    {
        // Don't log common conditions.
        if (ec != error::peer_disconnect && ec != error::operation_canceled)
        {
            LOGF("Websocket read failure [" << authority() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    // TODO: deserialize message from request_buffer and dispatch.
    ////distributor_.notify(message);
}

// pre-upgrade
// ----------------------------------------------------------------------------

void channel_ws::handle_read_request(const code& ec, size_t bytes,
    const http::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (upgraded_)
    {
        LOGP("Websocket is in upgraded state [" << authority() << "]");
        stop(network::error::operation_failed);
        return;
    }

    if (ec != error::upgraded)
    {
        channel_http::handle_read_request(ec, bytes, request);
        return;
    }

    upgraded_ = true;
    LOGP("Websocket upgraded [" << authority() << "]");

    const std::string welcome{ "Websocket libbitcoin/4.0" };
    send(to_chunk(welcome), false, [this](const code& ec) NOEXCEPT
    {
        // handle_send alread stops channel on ec.
        // One and only one handler of message must restart read loop.
        // In half duplex this happens only after send (ws full duplex).
        if (!ec) read_request();
    });
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
