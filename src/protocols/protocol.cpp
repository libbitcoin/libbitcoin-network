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
#include <bitcoin/network/protocols/protocol.hpp>

#include <algorithm>
#include <utility>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/peer/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol

using namespace system;
using namespace messages::peer;
using namespace std::placeholders;

// Protocols invoke channel stop for application layer protocol violations.
// Channels invoke channel stop for channel timouts and communcation failures.
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

// Zero if timer expired.
size_t protocol::remaining() const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "stranded");
    return channel_->remaining();
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

const network::settings& protocol::settings() const NOEXCEPT
{
    return session_->settings();
}

uint64_t protocol::identifier() const NOEXCEPT
{
    return channel_->identifier();
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
