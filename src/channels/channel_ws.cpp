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

void channel_ws::receive() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped() || paused() || !upgraded_)
    {
        channel_http::receive();
        return;
    }

    ws_read(request_buffer(),
        std::bind(&channel_ws::handle_receive_ws,
            shared_from_base<channel_ws>(), _1, _2));
}

// upgraded
// ----------------------------------------------------------------------------

void channel_ws::handle_receive_ws(const code& ec, size_t bytes) NOEXCEPT
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

    dispatch_ws(request_buffer(), bytes);
}

void channel_ws::dispatch_ws(const http::flat_buffer&,
    size_t LOG_ONLY(bytes)) NOEXCEPT
{
    LOGA("Websocket read of " << bytes  << " bytes [" << authority() << "]");

    // TODO: dispatch and restart upon completion.

    // Restart reader.
    receive();
}

// pre-upgrade
// ----------------------------------------------------------------------------

void channel_ws::handle_receive(const code& ec, size_t bytes,
    const http::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (upgraded_)
    {
        LOGF("Http request in websocket state [" << authority() << "]");
        stop(network::error::operation_failed);
        return;
    }

    if (ec != error::upgraded)
    {
        channel_http::handle_receive(ec, bytes, request);
        return;
    }

    upgraded_ = true;
    LOGA("Websocket upgraded [" << authority() << "]");
    receive();
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
