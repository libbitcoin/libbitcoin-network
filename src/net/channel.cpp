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
#include <bitcoin/network/net/channel.hpp>

#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages;
using namespace std::placeholders;

// Factory for fixed deadline timer pointer construction.
inline deadline::ptr timeout(const logger& log, asio::strand& strand,
    const deadline::duration& span) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return std::make_shared<deadline>(log, strand, span);
    BC_POP_WARNING()
}

// Factory for varied deadline timer pointer construction.
inline deadline::ptr expiration(const logger& log, asio::strand& strand,
    const deadline::duration& span) NOEXCEPT
{
    return timeout(log, strand, pseudo_random::duration(span));
}

channel::channel(const logger& log, const socket::ptr& socket,
    const settings& settings, uint64_t identifier, bool quiet) NOEXCEPT
  : proxy(socket),
    quiet_(quiet),
    settings_(settings),
    identifier_(identifier),
    expiration_(expiration(log, socket->strand(), settings.channel_expiration())),
    inactivity_(timeout(log, socket->strand(), settings.channel_inactivity())),
    negotiated_version_(settings.protocol_maximum),
    tracker<channel>(log)
{
}

channel::~channel() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "channel is not stopped");
    if (!stopped()) { LOGF("~channel is not stopped."); }
}

// Stop (started upon create).
// ----------------------------------------------------------------------------

void channel::stop(const code& ec) NOEXCEPT
{
    // Stop the read loop, stop accepting new work, cancel pending work.
    proxy::stop(ec);

    // Stop is posted to strand to protect timers.
    boost::asio::post(strand(),
        std::bind(&channel::do_stop, shared_from_base<channel>(), ec));
}

// This should not be called internally, as derived rely on stop() override.
void channel::do_stop(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_expiration();
    stop_inactivity();
}

// Pause/resume (paused upon create).
// ----------------------------------------------------------------------------

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

void channel::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    start_expiration();
    start_inactivity();
    proxy::resume();
}

// Properties.
// ----------------------------------------------------------------------------
// Version members are protected by the presumption of no reads during writes.
// Versions should only be set in handshake process, and only read thereafter.

bool channel::quiet() const NOEXCEPT
{
    return quiet_;
}

uint64_t channel::nonce() const NOEXCEPT
{
    return nonce_;
}

uint64_t channel::identifier() const NOEXCEPT
{
    return identifier_;
}

size_t channel::start_height() const NOEXCEPT
{
    return start_height_;
}

void channel::set_start_height(size_t height) NOEXCEPT
{
    BC_ASSERT_MSG(!is_limited<uint32_t>(height), "Time to upgrade protocol.");
    start_height_ = height;
}

uint32_t channel::negotiated_version() const NOEXCEPT
{
    return negotiated_version_;
}

void channel::set_negotiated_version(uint32_t value) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    negotiated_version_ = value;
}

// private
bool channel::is_handshaked() const NOEXCEPT
{
    return !is_null(peer_version_);
}

version::cptr channel::peer_version() const NOEXCEPT
{
    // peer_version_ defaults to nullptr, which implies not handshaked.
    return is_handshaked() ? peer_version_ : system::to_shared<messages::version>();
}

void channel::set_peer_version(const version::cptr& value) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    peer_version_ = value;
}

address_item_cptr channel::get_updated_address() const NOEXCEPT
{
    // Copy peer address.
    const auto peer = std::make_shared<address_item>(address());

    // Update timestamp, and services if handshaked.
    peer->timestamp = unix_time();
    if (is_handshaked())
        peer->services = peer_version_->services;

    return peer;
}

// Proxy overrides (channel maintains state for the proxy).
// ----------------------------------------------------------------------------
// These are const except for version (safe) and signal_activity (stranded).

size_t channel::minimum_buffer() const NOEXCEPT
{
    return settings_.minimum_buffer;
}

size_t channel::maximum_payload() const NOEXCEPT
{
    return settings_.maximum_payload();
}

uint32_t channel::protocol_magic() const NOEXCEPT
{
    return settings_.identifier;
}

bool channel::validate_checksum() const NOEXCEPT
{
    return settings_.validate_checksum;
}

uint32_t channel::version() const NOEXCEPT
{
    return negotiated_version();
}

// Cancels previous timer and retains configured duration.
void channel::signal_activity() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return start_inactivity();
}

// Timers.
// ----------------------------------------------------------------------------
// TODO: build DoS protection around rate_limit_, backlog(), total(), and time.
// A restarted timer invokes completion handler with error::operation_canceled.
// Called from start or strand.

void channel::stop_expiration() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    expiration_->stop();
}

void channel::start_expiration() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    // Handler is posted to the socket strand.
    expiration_->start(
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
    inactivity_->stop();
}

void channel::start_inactivity() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    // Handler is posted to the socket strand.
    inactivity_->start(
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

} // namespace network
} // namespace libbitcoin
