/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/protocols/protocol.hpp>

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

protocol::protocol(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : channel_(channel), session_(session), started_(false),
    reporter(session.log())
{
}

protocol::~protocol() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "protocol destruct before channel stop");
}

// Start/Stop.
// ----------------------------------------------------------------------------

void protocol::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "stranded");
    started_ = true;
}

bool protocol::started() const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "stranded");
    return started_;
}

// Utility to test for failure code or stopped.
// Any failure code from a send or receive handler implies channel::proxy stop.
// So this should be used to test entry into those handlers. Other handlers,
// such as timers, should use stopped() and independently evaluate the code.
bool protocol::stopped(const code& ec) const NOEXCEPT
{
    return channel_->stopped() || ec;
}

// Called from stop subscription instead of stop (which would be a cycle).
void protocol::stopping(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "stranded");
}

// Stop the channel::proxy, which results protocol stop handler invocation.
// The stop handler invokes stopping(ec), for protocol cleanup operations.
void protocol::stop(const code& ec) NOEXCEPT
{
    channel_->stop(ec);
}

// Suspend reads from the socket until resume.
void protocol::pause() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "stranded");
    channel_->pause();
}

////// Resume reads from the socket until pause or stop.
////void protocol::resume() NOEXCEPT
////{
////    BC_ASSERT_MSG(stranded(), "stranded");
////    channel_->resume();
////}

// Properties.
// ----------------------------------------------------------------------------
// The two public properties may be accessed outside the strand, but are never
// during handshake protocol operation. Thread safety requires that setters are
// never invoked outside of the handshake protocol (start handler).

const network::settings& protocol::settings() const NOEXCEPT
{
    return session_.settings();
}

bool protocol::stranded() const NOEXCEPT
{
    return channel_->stranded();
}

config::authority protocol::authority() const NOEXCEPT
{
    return channel_->authority();
}

config::authority protocol::origination() const NOEXCEPT
{
    return channel_->origination();
}

uint64_t protocol::nonce() const NOEXCEPT
{
    return channel_->nonce();
}

version::ptr protocol::peer_version() const NOEXCEPT
{
    return channel_->peer_version();
}

// Call only from handshake (version protocol), for thread safety.
void protocol::set_peer_version(const version::ptr& value) NOEXCEPT
{
    channel_->set_peer_version(value);
}

uint32_t protocol::negotiated_version() const NOEXCEPT
{
    return channel_->negotiated_version();
}

// Call only from handshake (version protocol), for thread safety.
void protocol::set_negotiated_version(uint32_t value) NOEXCEPT
{
    channel_->set_negotiated_version(value);
}

// Addresses.
// ----------------------------------------------------------------------------
// Address completion handlers are invoked on the channel strand.

void protocol::fetch(address_items_handler&& handler) NOEXCEPT
{
    session_.fetch(BIND3(do_fetch, _1, _2, std::move(handler)));
}

// Return to channel strand.
void protocol::do_fetch(const code& ec,
    const messages::address::ptr& message,
    const address_items_handler& handler) NOEXCEPT
{
    // TODO: use addresses pointer (copies addresses).
    boost::asio::post(channel_->strand(),
        BIND3(handle_fetch, ec, message, handler));
}

void protocol::handle_fetch(const code& ec,
    const messages::address::ptr& message,
    const address_items_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol");
    handler(ec, message);
}

void protocol::save(const address::ptr& message,
    count_handler&& handler) NOEXCEPT
{
    session_.save(message, BIND3(do_save, _1, _2, std::move(handler)));
}

// Return to channel strand.
void protocol::do_save(const code& ec, size_t accepted,
    const count_handler& handler) NOEXCEPT
{
    boost::asio::post(channel_->strand(),
        BIND3(handle_save, ec, accepted, std::move(handler)));
}

void protocol::handle_save(const code& ec, size_t accepted,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol");
    handler(ec, accepted);
}

// Send.
// ----------------------------------------------------------------------------
// Send (and receive) completion handlers are invoked on the channel strand.

// Send and receive failures are logged by the proxy, so there is no need to
// log here. This can be used as a no-op handler for sends. Some protocols may
// create custom handlers to perform operations upon send completion.
void protocol::handle_send(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
}

} // namespace network
} // namespace libbitcoin
