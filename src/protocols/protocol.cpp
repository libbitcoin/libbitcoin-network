/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

#include <algorithm>
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

#define CLASS protocol

using namespace system;
using namespace messages;
using namespace std::placeholders;

protocol::protocol(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : channel_(channel),
    session_(session),
    reporter(session->log)
{
}

protocol::~protocol() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "protocol destruct before channel stop");
    if (!stopped()) { LOGF("~protocol is not stopped."); }
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

// Resumes reads from the socket following pause.
void protocol::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "stranded");
    channel_->resume();
}

// Properties.
// ----------------------------------------------------------------------------
// The public properties may be accessed outside the strand, except during
// handshake protocol operation. Thread safety requires that setters are never
// invoked outside of the handshake protocol (start handler).

bool protocol::stranded() const NOEXCEPT
{
    return channel_->stranded();
}

config::authority protocol::authority() const NOEXCEPT
{
    return channel_->authority();
}

const config::address& protocol::outbound() const NOEXCEPT
{
    return channel_->address();
}

uint64_t protocol::nonce() const NOEXCEPT
{
    return channel_->nonce();
}

size_t protocol::start_height() const NOEXCEPT
{
    return channel_->start_height();
}

version::cptr protocol::peer_version() const NOEXCEPT
{
    return channel_->peer_version();
}

// Call only from handshake (version protocol), for thread safety.
void protocol::set_peer_version(const version::cptr& value) NOEXCEPT
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

const network::settings& protocol::settings() const NOEXCEPT
{
    return session_->settings();
}

address protocol::selfs() const NOEXCEPT
{
    const auto time_now = unix_time();
    const auto services = settings().services_maximum;
    const auto& selfs = settings().selfs;

    address message{};
    message.addresses.reserve(selfs.size());
    for (auto& self: selfs)
        message.addresses.push_back(self.to_address_item(time_now, services));

    return message;
}

uint64_t protocol::identifier() const NOEXCEPT
{
    return channel_->identifier();
}

// Addresses.
// ----------------------------------------------------------------------------
// Channel and network strands share same pool, and as long as a job is
// running in the pool, it will continue to accept work. Therefore handlers
// will not be orphaned during a stop as long as they remain in the pool.

size_t protocol::address_count() const NOEXCEPT
{
    return session_->address_count();
}

void protocol::fetch(address_handler&& handler) NOEXCEPT
{
    session_->fetch(
        BIND(handle_fetch, _1, _2, std::move(handler)));
}

void protocol::handle_fetch(const code& ec, const address_cptr& message,
    const address_handler& handler) NOEXCEPT
{
    // Return to channel strand.
    boost::asio::post(channel_->strand(),
        std::bind(handler, ec, message));
}

void protocol::save(const address_cptr& message,
    count_handler&& handler) NOEXCEPT
{
    session_->save(message,
        BIND(handle_save, _1, _2, std::move(handler)));
}

void protocol::handle_save(const code& ec, size_t accepted,
    const count_handler& handler) NOEXCEPT
{
    // Return to channel strand.
    boost::asio::post(channel_->strand(),
        std::bind(handler, ec, accepted));
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
