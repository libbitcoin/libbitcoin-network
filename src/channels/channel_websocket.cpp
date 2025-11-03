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
#include <bitcoin/network/channels/channel_websocket.hpp>

#include <optional>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/http.hpp>

namespace libbitcoin {
namespace network {

#define CLASS channel_websocket

using namespace system;
using namespace network::http;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// public/virtual
// ----------------------------------------------------------------------------

void channel_websocket::read_request() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped() || paused() || !upgraded_)
    {
        channel_http::read_request();
        return;
    }

    ws_read(request_buffer(),
        std::bind(&channel_websocket::handle_read_websocket,
            shared_from_base<channel_websocket>(), _1, _2));
}

// upgraded
// ----------------------------------------------------------------------------

void channel_websocket::handle_read_websocket(const code& ec,
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

    // TODO: deserialize websocket message from request_buffer and dispatch.
    ////distributor_.notify(message);
    resume();
}

// pre-upgrade
// ----------------------------------------------------------------------------

void channel_websocket::handle_read_request(const code& ec, size_t bytes,
    const http::request_cptr& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec || stopped() || !is_websocket_upgrade(*request))
    {
        channel_http::handle_read_request(ec, bytes, request);
        return;
    }

    send_websocket_accept(*request);
}

void channel_websocket::send_websocket_accept(const request& request) NOEXCEPT
{
    BC_ASSERT(stranded());

    response out{ status::switching_protocols, request.version() };
    out.set(field::upgrade, "websocket");
    out.set(field::connection, "upgrade");
    out.set(field::sec_websocket_accept, to_websocket_accept(request));
    out.prepare_payload();

    result_handler complete =
        std::bind(&channel_websocket::handle_upgrade_complete,
            shared_from_base<channel_websocket>(), _1);

    channel_http::send(std::move(out), std::move(complete));
}

void channel_websocket::handle_upgrade_complete(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        LOGQ("Websocket read abort [" << authority() << "]");
        return;
    }

    if (ec)
    {
        LOGP("Websocket upgrade failed [" << authority() << "] "
            << ec.message());

        stop(ec);
        return;
    }

    upgraded_ = true;
    set_websocket();
    LOGP("Websocket upgraded [" << authority() << "]");

    resume();
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
