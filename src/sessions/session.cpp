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
#include <bitcoin/network/sessions/session.hpp>

#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocols.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session

using namespace system;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session::session(p2p& network) NOEXCEPT
  : network_(network),
    stopped_(true),
    timeout_(network.network_settings().retry_timeout()),
    stop_subscriber_(network.strand()),
    defer_subscriber_(network.strand()),
    pend_subscriber_(network.strand()),
    reporter(network.log())
{
}

session::~session() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "The session was not stopped.");
}

void session::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    stopped_.store(false, std::memory_order_relaxed);
    handler(error::success);
}

void session::stop() NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    stopped_.store(true, std::memory_order_relaxed);
    defer_subscriber_.stop(error::service_stopped);
    pend_subscriber_.stop(error::service_stopped);
    stop_subscriber_.stop(error::service_stopped);
}

// Channel sequence.
// ----------------------------------------------------------------------------

void session::start_channel(const channel::ptr& channel,
    result_handler&& started, result_handler&& stopped) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (this->stopped())
    {
        channel->stop(error::service_stopped);
        started(error::service_stopped);
        stopped(error::service_stopped);
        return;
    }

    // In case of a loopback, inbound and outbound are on the same strand.
    // Inbound does not check nonce until handshake completes, so no race.
    if (!inbound() && !network_.store_nonce(channel->nonce()))
    {
        channel->stop(error::channel_conflict);
        started(error::channel_conflict);
        stopped(error::channel_conflict);
        return;
    }

    // Pend channel for handshake duration (for quick stop).
    pend(channel);

    result_handler start =
        BIND4(handle_channel_start, _1, channel, std::move(started),
            std::move(stopped));

    result_handler shake =
        BIND3(handle_handshake, _1, channel, std::move(start));

    // Switch to channel context.
    boost::asio::post(channel->strand(),
        BIND2(do_attach_handshake, channel, std::move(shake)));
}

void session::do_attach_handshake(const channel::ptr& channel,
    const result_handler& handshake) const NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for handshake attach");

    attach_handshake(channel, move_copy(handshake));

    // Channel is started/paused upon creation, this begins the read loop.
    channel->resume();
}

void session::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) const NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for handshake attach");

    // Weak reference safe as sessions outlive protocols.
    const auto& self = *this;
    const auto enable_reject = settings().enable_reject;
    const auto maximum_version = settings().protocol_maximum;

    // Protocol must pause the channel after receiving version and verack.

    // Reject is supported starting at bip61 (70002) and later deprecated.
    if (enable_reject && maximum_version >= messages::level::bip61)
        channel->attach<protocol_version_70002>(self)
            ->shake(std::move(handler));

    // Relay is supported starting at bip37 (70001).
    else if (maximum_version >= messages::level::bip37)
        channel->attach<protocol_version_70001>(self)
            ->shake(std::move(handler));

    else
        channel->attach<protocol_version_31402>(self)
            ->shake(std::move(handler));
}

void session::handle_handshake(const code& ec, const channel::ptr& channel,
    const result_handler& start) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");

    // Return to network context.
    boost::asio::post(network_.strand(),
        BIND3(do_handle_handshake, ec, channel, start));
}

void session::do_handle_handshake(const code& ec, const channel::ptr& channel,
    const result_handler& start) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (!unpend(channel))
    {
        LOG("Unpend failed to locate channel.");
    }

    // Handles channel stopped or protocol start results code.
    // This retains the channel and allows broadcasts, stored if no code.
    start(ec ? ec : network_.store_channel(channel, notify(), inbound()));
}

// Context free method.
void session::handle_channel_start(const code& ec, const channel::ptr& channel,
    const result_handler& started, const result_handler& stopped) NOEXCEPT
{
    // Handles network_.store, channel stopped, and protocol start code.
    if (ec)
    {
        channel->stop(ec);

        // Unstore fails on counter underflows (implies bug).
        if (const auto error_code = network_.unstore_channel(channel, inbound()))
        {
            LOG("Unstore on channel start failed: " << error_code.message());
        }

        started(ec);
        stopped(ec);
        return;
    }

    // Capture the channel stop handler in the channel.
    // If stopped, or upon channel stop, handler is invoked.
    channel->subscribe_stop(
        BIND3(handle_channel_stopped, _1, channel, stopped),
        BIND3(handle_channel_started, _1, channel, started));
}

void session::handle_channel_started(const code& ec,
    const channel::ptr& channel, const result_handler& started) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");

    // Return to network context.
    boost::asio::post(network_.strand(),
        BIND3(do_handle_channel_started, ec, channel, started));
}

void session::do_handle_channel_started(const code& ec,
    const channel::ptr& channel, const result_handler& started) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    // Handles channel subscribe_stop code.
    started(ec);

    // Do not attach protocols if failed.
    if (ec)
        return;

    // Switch to channel context.
    boost::asio::post(channel->strand(),
        BIND1(do_attach_protocols, channel));
}

void session::do_attach_protocols(const channel::ptr& channel) const NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for protocol attach");

    attach_protocols(channel);

    // Resume accepting messages on the channel, timers restarted.
    channel->resume();
}

// Override in derived sessions to attach protocols.
void session::attach_protocols(const channel::ptr& channel) const NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for protocol attach");

    // Weak reference safe as sessions outlive protocols.
    const auto& self = *this;
    const auto enable_alert = settings().enable_alert;
    const auto enable_reject = settings().enable_reject;
    const auto negotiated_version = channel->negotiated_version();

    if (negotiated_version >= messages::level::bip31)
        channel->attach<protocol_ping_60001>(self)->start();
    else
        channel->attach<protocol_ping_31402>(self)->start();

    // Alert is deprecated.
    if (enable_alert)
        channel->attach<protocol_alert_31402>(self)->start();

    // Reject is supported starting at bip61 (70002) and later deprecated.
    if (enable_reject && negotiated_version >= messages::level::bip61)
        channel->attach<protocol_reject_70002>(self)->start();

    channel->attach<protocol_address_31402>(self)->start();
}

void session::handle_channel_stopped(const code& ec,
    const channel::ptr& channel, const result_handler& stopped) NOEXCEPT
{
    // Return to network context.
    boost::asio::post(network_.strand(),
        BIND3(do_handle_channel_stopped, ec, channel, stopped));
}

void session::do_handle_channel_stopped(const code& ec,
    const channel::ptr& channel, const result_handler& stopped) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    // Unstore fails on counter underflows (implies bug).
    if (const auto error_code = network_.unstore_channel(channel, inbound()))
    {
        LOG("Unstore on channel stop failed: " << error_code.message());
    }

    // Unpend outgoing nonce (false implies bug). This in stop vs. handshake
    // to avoid loopback race, with in/outbound channels on the same strand.
    if (!inbound() && !network_.unstore_nonce(channel->nonce()))
    {
        LOG("Unpend failed to locate channel nonce.");
    }

    // Assume stop notification, but may be subscribe failure (idempotent).
    // Handles stop reason code, stop subscribe failure or stop notification.
    stopped(ec);
}

// Subscriptions.
// ----------------------------------------------------------------------------

void session::defer(result_handler&& handler, const uintptr_t& id) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    // Subscribe completes before handle_timer can be invoked (timer stranded).
    const auto timer = std::make_shared<deadline>(log(), network_.strand());
    timer->start(BIND3(handle_timer, _1, id, std::move(handler)), timeout_);
    defer_subscriber_.subscribe(BIND3(handle_defer, _1, id, timer), id);
}

void session::handle_timer(const code& ec, uintptr_t id,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    ////LOG("Delay timer (" << id << ") notify: " << ec.message());
    defer_subscriber_.notify_one(id, ec);
    complete(ec);
}

bool session::handle_defer(const code&, uintptr_t,
    const deadline::ptr& timer) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    ////LOG("Delay timer (" << id << ") stop: " << ec.message());
    timer->stop();
    return false;
}

void session::pend(const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    ////LOG("Pending channel (" << channel->nonce() << ").");
    pend_subscriber_.subscribe(BIND2(handle_pend, _1, channel), channel);
}

bool session::handle_pend(const code& ec, const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    if (ec) channel->stop(ec);
    return false;
}

bool session::unpend(const channel::ptr& channel) NOEXCEPT
{
    // Ok to not find after stop, clears before channel stop handlers fire.
    const auto result = pend_subscriber_.notify_one(channel, error::success);
    ////LOG("Unpending channel (" << channel->nonce() << ").");
    return result.first || stopped();
}

void session::subscribe_stop(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    stop_subscriber_.subscribe(std::move(handler));
}

// Factories.
// ----------------------------------------------------------------------------

acceptor::ptr session::create_acceptor() NOEXCEPT
{
    return network_.create_acceptor();
}

connector::ptr session::create_connector() NOEXCEPT
{
    return network_.create_connector();
}

connectors_ptr session::create_connectors(size_t count) NOEXCEPT
{
    return network_.create_connectors(count);
}

// Properties.
// ----------------------------------------------------------------------------

bool session::stopped() const NOEXCEPT
{
    return stopped_.load(std::memory_order_relaxed);
}

bool session::stranded() const NOEXCEPT
{
    return network_.stranded();
}

size_t session::address_count() const NOEXCEPT
{
    return network_.address_count();
}

size_t session::channel_count() const NOEXCEPT
{
    return network_.channel_count();
}

size_t session::inbound_channel_count() const NOEXCEPT
{
    return network_.inbound_channel_count();
}

size_t session::outbound_channel_count() const NOEXCEPT
{
    return floored_subtract(channel_count(), inbound_channel_count());
}

bool session::disabled(const config::address& peer) const NOEXCEPT
{
    return !settings().enable_ipv6 && peer.is_v6();
}

bool session::insufficient(const config::address& address) const NOEXCEPT
{
    return (address.item().services & settings().services_minimum) !=
        settings().services_minimum;
}

bool session::unsupported(const config::address& address) const NOEXCEPT
{
    return to_bool(address.item().services & settings().invalid_services);
}

bool session::whitelisted(const config::authority& authority) const NOEXCEPT
{
    return settings().whitelists.empty() ||
        contains(settings().whitelists, authority);
}

bool session::blacklisted(const config::authority& authority) const NOEXCEPT
{
    return contains(settings().blacklists, authority);
}

const network::settings& session::settings() const NOEXCEPT
{
    return network_.network_settings();
}

// Utilities.
// ----------------------------------------------------------------------------
// stackoverflow.com/questions/57411283/
// calling-non-const-function-of-another-class-by-reference-from-const-function

void session::take(address_item_handler&& handler) const NOEXCEPT
{
    network_.take(std::move(handler));
}

void session::fetch(address_handler&& handler) const NOEXCEPT
{
    network_.fetch(std::move(handler));
}

void session::restore(const address_item_cptr& address,
    result_handler&& handler) const NOEXCEPT
{
    network_.restore(address, std::move(handler));
}

void session::save(const address_cptr& message,
    count_handler&& handler) const NOEXCEPT
{
    network_.save(message, std::move(handler));
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
