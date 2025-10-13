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
#include <bitcoin/network/net/channel.hpp>

#include <memory>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Factory for fixed deadline timer pointer construction (or null).
inline deadline::ptr make_timer(const logger& log, asio::strand& strand,
    const deadline::duration& span) NOEXCEPT
{
    return to_bool(span.count()) ?
        std::make_shared<deadline>(log, strand, span) : nullptr;
}

// Protocols invoke channel stop for application layer protocol violations.
// Channels invoke channel stop for channel timouts and communcation failures.
channel::channel(const logger& log, const socket::ptr& socket,
    const network::settings& settings, uint64_t identifier,
    const deadline::duration& inactivity,
    const deadline::duration& expiration) NOEXCEPT
  : proxy(socket),
    settings_(settings),
    identifier_(identifier),
    inactivity_(make_timer(log, socket->strand(), inactivity)),
    expiration_(make_timer(log, socket->strand(), expiration))
{
}

channel::~channel() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "channel is not stopped");
    if (!stopped()) { LOGF("~channel is not stopped."); }
}

// Start/stop/resume (started/paused upon create).
// ----------------------------------------------------------------------------

// This should not be called internally.
void channel::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_expiration();
    stop_inactivity();
    proxy::stopping(ec);
}

// Timers are set for handshake and reset upon protocol start.
// Version protocols may have more restrictive completion timeouts.
// A restarted timer invokes completion handler with error::operation_canceled.

void channel::pause() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_expiration();
    stop_inactivity();
    proxy::pause();
}

// Resume timers from pause and start read loop.
void channel::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    start_expiration();
    start_inactivity();
    proxy::resume();
}

// Timers.
// ----------------------------------------------------------------------------
// TODO: build DoS protection around rate_limit_, backlog(), total(), and time.
// A restarted timer invokes completion handler with error::operation_canceled.
// Called from start or strand.

// protected
void channel::waiting() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    start_inactivity();
}

size_t channel::remaining() const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return inactivity_ ? limit<size_t>(inactivity_->remaining().count()) : zero;
}

void channel::stop_expiration() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    if (expiration_) expiration_->stop();
}

void channel::start_expiration() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    // Handler is posted to the socket strand.
    if (expiration_) expiration_->start(
        std::bind(&channel::handle_expiration,
            shared_from_base<channel>(), _1));
}

void channel::handle_expiration(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // error::operation_canceled is set by timer reset (channel not stopped).
    if (stopped() || ec == error::operation_canceled)
        return;

    if (ec)
    {
        LOGF("Lifetime timer fail [" << authority() << "] " << ec.message());
        stop(ec);
        return;
    }

    stop(error::channel_expired);
}

void channel::stop_inactivity() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    if (inactivity_) inactivity_->stop();
}

// Cancels previous timer and retains configured duration.
void channel::start_inactivity() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    // Handler is posted to the socket strand.
    if (inactivity_) inactivity_->start(
        std::bind(&channel::handle_inactivity,
            shared_from_base<channel>(), _1));
}

// There is no timeout set on individual sends and receives, just inactivity.
void channel::handle_inactivity(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // error::operation_canceled is set by timer reset (channel not stopped).
    if (stopped() || ec == error::operation_canceled)
        return;

    if (ec)
    {
        LOGF("Inactivity timer fail [" << authority() << "] " << ec.message());
        stop(ec);
        return;
    }

    stop(error::channel_inactive);
}

// Properties.
// ----------------------------------------------------------------------------

uint64_t channel::nonce() const NOEXCEPT
{
    return nonce_;
}

uint64_t channel::identifier() const NOEXCEPT
{
    return identifier_;
}

const network::settings& channel::settings() const NOEXCEPT
{
    return settings_;
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
