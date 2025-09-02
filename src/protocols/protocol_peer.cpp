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
#include <bitcoin/network/protocols/protocol_peer.hpp>

#include <algorithm>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_peer

using namespace system;
using namespace messages;
using namespace std::placeholders;

protocol_peer::protocol_peer(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    channel_(std::dynamic_pointer_cast<channel_peer>(channel)),
    session_(std::dynamic_pointer_cast<session_peer>(session))
{
}

// Properties.
// ----------------------------------------------------------------------------
// The public properties may be accessed outside the strand, except during
// handshake protocol operation. Thread safety requires that setters are never
// invoked outside of the handshake protocol (start handler).

size_t protocol_peer::start_height() const NOEXCEPT
{
    return channel_->start_height();
}

version::cptr protocol_peer::peer_version() const NOEXCEPT
{
    return channel_->peer_version();
}

// Call only from handshake (version protocol), for thread safety.
void protocol_peer::set_peer_version(const version::cptr& value) NOEXCEPT
{
    channel_->set_peer_version(value);
}

uint32_t protocol_peer::negotiated_version() const NOEXCEPT
{
    return channel_->negotiated_version();
}

// Call only from handshake (version protocol), for thread safety.
void protocol_peer::set_negotiated_version(uint32_t value) NOEXCEPT
{
    channel_->set_negotiated_version(value);
}

address protocol_peer::selfs() const NOEXCEPT
{
    const auto time_now = unix_time();
    const auto services = settings().services_maximum;
    const auto& selfs = settings().selfs;

    address message{};
    message.addresses.reserve(selfs.size());
    for (const auto& self: selfs)
        message.addresses.push_back(self.to_address_item(time_now, services));

    return message;
}

// Addresses.
// ----------------------------------------------------------------------------
// Channel and network strands share same pool, and as long as a job is
// running in the pool, it will continue to accept work. Therefore handlers
// will not be orphaned during a stop as long as they remain in the pool.

size_t protocol_peer::address_count() const NOEXCEPT
{
    return session_->address_count();
}

void protocol_peer::fetch(address_handler&& handler) NOEXCEPT
{
    session_->fetch(
        BIND(handle_fetch, _1, _2, std::move(handler)));
}

void protocol_peer::handle_fetch(const code& ec, const address_cptr& message,
    const address_handler& handler) NOEXCEPT
{
    // Return to channel strand.
    boost::asio::post(channel_->strand(),
        std::bind(handler, ec, message));
}

void protocol_peer::save(const address_cptr& message,
    count_handler&& handler) NOEXCEPT
{
    session_->save(message,
        BIND(handle_save, _1, _2, std::move(handler)));
}

void protocol_peer::handle_save(const code& ec, size_t accepted,
    const count_handler& handler) NOEXCEPT
{
    // Return to channel strand.
    boost::asio::post(channel_->strand(),
        std::bind(handler, ec, accepted));
}

} // namespace network
} // namespace libbitcoin
