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

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// Helper to derive maximum message payload size from settings.
inline size_t payload_maximum(const settings& settings)
{
    return heading::maximum_payload_size(settings.protocol_maximum,
        to_bool(settings.services & service::node_witness));
}

// Factory for fixed deadline timer pointer construction.
inline deadline::ptr timeout(asio::strand& strand, const duration& span)
{
    return std::make_shared<deadline>(strand, span);
}

// Factory for varied deadline timer pointer construction.
inline deadline::ptr expiration(asio::strand& strand, const duration& span)
{
    return timeout(strand, pseudo_random::duration(span));
}

// TODO: implement logging in the same manner as tracking, passing the shared
// TODO: log instance on construct of classes derived from a logger class.
// TODO: logger exposes read/write methods and can be specialized using an
// TODO: intermediate logger, such as channel_logger so that formatting can
// TODO: be isolated from class logic. 
//
// TODO: Give the shared log instance a strand for each "file" that it writes,
// TODO: such as standard out. Place the strand in a low priority threadpool
// TODO: owned by log, with its own start/stop methods. This can bypass the
// TODO: string serialization when a particular log level is turned off, with
// TODO: log calls short-circuited. The strand preserves message order while
// TODO: preserving concurrency across the threadpool.
//
// TODO: I/O can be specialized in construction of the log instance and even
// TODO: injected to the network instance on construct, allowing for dynamic
// TODO: log control of a running instance and injection of a custom log sink.
//
// TODO: The server could then sink log messages directly to ZMQ subscribers.
// TODO: This can then easily be shipped out of process or even over the net.
// TODO: Moving it out of the process allows log processing to operate on an
// TODO: external buffer and CPU, while remaining dynamic. Client can implement
// TODO: the log stub for other process, and BX can then sink messages from BS.
// TODO: Multiple instances of BX can sink different endpoints, such as one for
// TODO: each type of log message.
//
// TODO: Server can also accept control messages through ZeroMQ query, allowing
// TODO: it to dynamically alter state, including log levels. This allows one
// TODO: enable and view log output without restarting the box, or to force a
// TODO: checkpoint flush, enable endpoints, change configurable state, etc.
// TODO: This can be managed through an updateable version of settings.
//
// TODO: Can also implement settings using the same technique as log. Pass the
// TODO: shared settings instance on construct and to an intermediate base.
// TODO: have the base retain the reference and expose values directly, and
// TODO: compute values from them, without cluttering constructors and without
// TODO: copying individual values. More importantly this allows the values to
// TODO: be dynamically-updated, and for the instance to handle a configuration
// TODO: change event, filtered for the type or instance. The log and settings
// TODO: instances can be injected through a single configuration object.
//
// TODO: So toss boost:log and remove from dependencies. First implement a
// TODO: simple console sink.

channel::channel(socket::ptr socket, const settings& settings)
  : proxy(socket),
    maximum_payload_(payload_maximum(settings)),
    protocol_magic_(settings.identifier),
    channel_nonce_(pseudo_random::next<uint64_t>(1, max_uint64)),
    validate_checksum_(settings.validate_checksum),
    verbose_logging_(settings.verbose),
    negotiated_version_(settings.protocol_maximum),
    peer_version_(std::make_shared<messages::version>()),
    expiration_(expiration(socket->strand(), settings.channel_expiration())),
    inactivity_(timeout(socket->strand(), settings.channel_inactivity()))
{
}

channel::~channel()
{
    BC_ASSERT_MSG(stopped(), "channel is not stopped");
}

// Stop (started upon create).
// ----------------------------------------------------------------------------

void channel::stop(const code& ec)
{
    // Stop is dispatched to strand to protect timers.
    boost::asio::dispatch(strand(),
        std::bind(&channel::do_stop,
            shared_from_base<channel>(), ec));
}

// This should not be called internally, as derived rely on stop() override.
void channel::do_stop(const code& ec)
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
void channel::resume()
{
    start_expiration();
    start_inactivity();
    proxy::resume();
}

// Properties.
// ----------------------------------------------------------------------------
// Version members are protected by the presumption of no reads during writes.
// Versions should only be set in handshake process, and only read thereafter.

// Member is const.
uint64_t channel::nonce() const noexcept
{
    return channel_nonce_;
}

uint32_t channel::negotiated_version() const noexcept
{
    return negotiated_version_;
}

void channel::set_negotiated_version(uint32_t value) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    negotiated_version_ = value;
}

version::ptr channel::peer_version() const noexcept
{
    return peer_version_;
}

void channel::set_peer_version(version::ptr value) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    peer_version_ = value;
}

// Proxy overrides (channel maintains state for the proxy).
// ----------------------------------------------------------------------------
// These are const except for version (safe) and signal_activity (stranded).

size_t channel::maximum_payload() const noexcept
{
    return maximum_payload_;
}

uint32_t channel::protocol_magic() const noexcept
{
    return protocol_magic_;
}

bool channel::validate_checksum() const noexcept
{
    return validate_checksum_;
}

bool channel::verbose() const noexcept
{
    return verbose_logging_;
}

uint32_t channel::version() const noexcept
{
    return negotiated_version();
}

// Cancels previous timer and retains configured duration.
// A canceled timer does not invoke its completion handler.
void channel::signal_activity()
{
    BC_ASSERT_MSG(stranded(), "strand");
    return start_inactivity();
}

// Timers.
// ----------------------------------------------------------------------------

// Called from start or strand.
void channel::start_expiration()
{
    if (stopped())
        return;

    // Handler is posted to the socket strand.
    expiration_->start(
        std::bind(&channel::handle_expiration,
            shared_from_base<channel>(), _1));
}

void channel::handle_expiration(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped() || ec == error::operation_canceled)
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Channel lifetime timer failure [" << authority() << "] "
            << ec.message() << std::endl;
        stop(ec);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Channel lifetime expired [" << authority() << "]" << std::endl;
    stop(ec);
}

// Called from start or strand.
void channel::start_inactivity()
{
    if (stopped())
        return;

    // Handler is posted to the socket strand.
    inactivity_->start(
        std::bind(&channel::handle_inactivity,
            shared_from_base<channel>(), _1));
}

// There is no timeout set on individual sends and receives, just inactivity.
void channel::handle_inactivity(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped() || ec == error::operation_canceled)
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Channel inactivity timer failure [" << authority() << "] "
            << ec.message() << std::endl;
        stop(ec);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Channel inactivity timeout [" << authority() << "]" << std::endl;
    stop(ec);
}

} // namespace network
} // namespace libbitcoin
