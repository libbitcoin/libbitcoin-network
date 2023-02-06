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
#include <bitcoin/network/net/channel.hpp>

#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages;
using namespace std::placeholders;

// Helper to derive maximum message payload size from settings.
inline size_t payload_maximum(const settings& settings) NOEXCEPT
{
    return heading::maximum_payload_size(settings.protocol_maximum,
        to_bool(settings.services_maximum & service::node_witness));
}

// Factory for fixed deadline timer pointer construction.
inline deadline::ptr timeout(const logger& log, asio::strand& strand,
    const duration& span) NOEXCEPT
{
    return std::make_shared<deadline>(log, strand, span);
}

// Factory for varied deadline timer pointer construction.
inline deadline::ptr expiration(const logger& log, asio::strand& strand,
    const duration& span) NOEXCEPT
{
    return timeout(log, strand, pseudo_random::duration(span));
}

channel::channel(const logger& log, const socket::ptr& socket,
    const settings& settings) NOEXCEPT
  : channel(log, socket, settings, socket->authority().to_address_item())
{
}

channel::channel(const logger& log, const socket::ptr& socket,
    const settings& settings, const config::address& address) NOEXCEPT
  : proxy(socket),
    ////rate_limit_(settings.rate_limit),
    maximum_payload_(payload_maximum(settings)),
    protocol_magic_(settings.identifier),
    channel_nonce_(pseudo_random::next<uint64_t>(one, max_uint64)),
    validate_checksum_(settings.validate_checksum),
    address_(address),
    negotiated_version_(settings.protocol_maximum),
    peer_version_(to_shared<messages::version>()),
    expiration_(expiration(log, socket->strand(), settings.channel_expiration())),
    inactivity_(timeout(log, socket->strand(), settings.channel_inactivity())),
    tracker<channel>(log)
{
}

channel::~channel() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "channel is not stopped");
}

// Stop (started upon create).
// ----------------------------------------------------------------------------

void channel::stop(const code& ec) NOEXCEPT
{
    // Stop is dispatched to strand to protect timers.
    boost::asio::dispatch(strand(),
        std::bind(&channel::do_stop,
            shared_from_base<channel>(), ec));
}

// This should not be called internally, as derived rely on stop() override.
void channel::do_stop(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    proxy::stop(ec);
    inactivity_->stop();
    expiration_->stop();
}

// Pause/resume (paused upon create).
// ----------------------------------------------------------------------------

// Timers are set for handshake and reset upon protocol start.
// Version protocols may have more restrictive completion timeouts.
// A restarted timer invokes completion handler with error::operation_canceled.
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

// Member is const.
uint64_t channel::nonce() const NOEXCEPT
{
    return channel_nonce_;
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

version::cptr channel::peer_version() const NOEXCEPT
{
    return peer_version_;
}

void channel::set_peer_version(const version::cptr& value) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    peer_version_ = value;
}

const config::address& channel::address() const NOEXCEPT
{
    return address_;
}

address_item_cptr channel::updated_address() const NOEXCEPT
{
    // clang doesn't like vargs construct.
    return to_shared(address_item{ unix_time(), peer_version()->services,
        address_.item().ip, address_.item().port });
}

// Proxy overrides (channel maintains state for the proxy).
// ----------------------------------------------------------------------------
// These are const except for version (safe) and signal_activity (stranded).

size_t channel::maximum_payload() const NOEXCEPT
{
    return maximum_payload_;
}

uint32_t channel::protocol_magic() const NOEXCEPT
{
    return protocol_magic_;
}

bool channel::validate_checksum() const NOEXCEPT
{
    return validate_checksum_;
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

// Called from start or strand.
// A restarted timer invokes completion handler with error::operation_canceled.
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
        LOG("Lifetime timer fail [" << authority() << "] " << ec.message());
        stop(ec);
        return;
    }

    stop(error::channel_expired);
}

// Called from start or strand.
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
        LOG("Inactivity timer fail [" << authority() << "] " << ec.message());
        stop(ec);
        return;
    }

    stop(error::channel_inactive);
}

} // namespace network
} // namespace libbitcoin
