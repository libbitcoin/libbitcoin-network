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
#include <bitcoin/network/protocols/protocol_client.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_client

using namespace asio;
using namespace messages::rpc;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_client::protocol_client(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    channel_(std::dynamic_pointer_cast<channel_client>(channel)),
    session_(std::dynamic_pointer_cast<session_client>(session)),
    tracker<protocol_client>(session->log)
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_client::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_client");

    if (started())
        return;

    SUBSCRIBE_CHANNEL(http_request, handle_receive_request, _1, _2);
    protocol::start();
}

// Inbound/outbound.
// ----------------------------------------------------------------------------

void protocol_client::handle_receive_request(const code& ec,
    const http_request&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_client");

    if (stopped(ec))
        return;

    static std::atomic_size_t count{};

    using namespace boost::beast;
    http_response response{ http::status::ok, 11 };
    response.set(http::field::content_type, "text/plain");
    response.body() = "Hello, World!\r\n#";
    response.body() += system::serialize(count++);
    response.prepare_payload();

    SEND(response, handle_send, _1);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
