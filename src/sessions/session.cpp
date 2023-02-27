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
#include <bitcoin/network/log/log.hpp>
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
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session::session(p2p& network, uint64_t identifier) NOEXCEPT
  : network_(network),
    identifier_(identifier),
    stop_subscriber_(network.strand()),
    reporter(network.log())
{
}

session::~session() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "The session was not stopped.");
    if (!stopped()) { LOGF("~session is not stopped."); }
}

void session::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    stopped_.store(false);
    handler(error::success);
}

void session::stop() NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    stopped_.store(true);
    stop_subscriber_.stop(error::service_stopped);
}

// Channel sequence.
// ----------------------------------------------------------------------------

void session::start_channel(const channel::ptr& channel,
    result_handler&& starter, result_handler&& stopper) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (stopped())
    {
        // Direct invoke of handlers.
        channel->stop(error::service_stopped);
        starter(error::service_stopped);
        stopper(error::service_stopped);
        return;
    }

    // In case of a loopback, inbound and outbound are on the same strand.
    // Inbound does not check nonce until handshake completes, so no race.
    if (!network_.store_nonce(*channel))
    {
        // Direct invoke of handlers (continuing).
        channel->stop(error::channel_conflict);
        starter(error::channel_conflict);
        stopper(error::channel_conflict);
        return;
    }

    // Pend channel for handshake duration (for quick stop).
    pend(channel);

    result_handler start =
        BIND4(handle_channel_start, _1, channel, std::move(starter),
            std::move(stopper));

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
    const auto maximum_version = settings().protocol_maximum;
    const auto extended_version = maximum_version >= messages::level::bip37;
    const auto enable_reject = settings().enable_reject &&
        maximum_version >= messages::level::bip61;

    // Protocol must pause the channel after receiving version and verack.

    // Reject is deprecated.
    if (enable_reject)
        channel->attach<protocol_version_70002>(self)
            ->shake(std::move(handler));

    else if (extended_version)
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

    // Unpend channel from handshake.
    unpend(channel);

    // Handles channel and protocol start failures.
    if (ec)
    {
        network_.unstore_nonce(*channel);
        channel->stop(ec);
        start(ec);
        return;
    }

    // TODO: Store currently "re-pends" the channel on a vector.
    // This should be replaced by p2p broadcast/stop subscription.
    if (const auto code = network_.count_channel(channel))
    {
        network_.unstore_nonce(*channel);
        channel->stop(code);
        start(code);
        return;
    }

    // Requires uncount_channel/unstore_nonce on stop if and only if success.
    start(ec);
}

void session::handle_channel_start(const code& ec, const channel::ptr& channel,
    const result_handler& started, const result_handler& stopped) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (ec)
    {
        started(ec);
        stopped(ec);
        return;
    }

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
    const auto enable_address = settings().enable_address;
    const auto negotiated_version = channel->negotiated_version();
    const auto enable_pong = negotiated_version >= messages::level::bip31;
    const auto enable_reject = settings().enable_reject &&
        negotiated_version >= messages::level::bip61;

    if (enable_pong)
        channel->attach<protocol_ping_60001>(self)->start();
    else
        channel->attach<protocol_ping_31402>(self)->start();

    // Alert is deprecated.
    if (enable_alert)
        channel->attach<protocol_alert_31402>(self)->start();

    // Reject is deprecated.
    if (enable_reject)
        channel->attach<protocol_reject_70002>(self)->start();

    if (enable_address)
    {
        channel->attach<protocol_address_in_31402>(self)->start();
        channel->attach<protocol_address_out_31402>(self)->start();
    }
}

void session::handle_channel_stopped(const code& ec,
    const channel::ptr& channel, const result_handler& stopped) NOEXCEPT
{
    // Return to network context.
    boost::asio::post(network_.strand(),
        BIND3(do_handle_channel_stopped, ec, channel, stopped));
}

// Unnonce in stop vs. handshake to avoid loopback race (in/out same strand).
void session::do_handle_channel_stopped(const code& ec,
    const channel::ptr& channel, const result_handler& stopped) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    network_.uncount_channel(channel);
    network_.unstore_nonce(*channel);

    // Assume stop notification, but may be subscribe failure (idempotent).
    // Handles stop reason code, stop subscribe failure or stop notification.
    stopped(ec);
}

// Subscriptions.
// ----------------------------------------------------------------------------

// private
void session::defer(const steady_clock::duration& timeout,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    const auto key = create_key();
    const auto timer = std::make_shared<deadline>(log(), network_.strand());

    timer->start(
        BIND3(handle_timer, _1, key, std::move(handler)), timeout);

    stop_subscriber_.subscribe(
        BIND3(handle_defer, _1, key, timer), key);
}

void session::defer(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    defer(settings().retry_timeout(), std::move(handler));
}

void session::defer_address_starvation(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    defer(settings().connect_timeout(), std::move(handler));
}

void session::handle_timer(const code& ec, object_key key,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    stop_subscriber_.notify_one(key, ec);
    complete(ec);
}

bool session::handle_defer(const code&, object_key,
    const deadline::ptr& timer) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    timer->stop();
    return false;
}

void session::pend(const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    stop_subscriber_.subscribe(BIND2(handle_pend, _1, channel),
        channel->identifier());
}

// Ok to not find after stop, clears before channel stop handlers fire.
void session::unpend(const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    notify(channel->identifier());
}

bool session::handle_pend(const code& ec, const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    // error::success prevents channel stop.
    if (ec)
        channel->stop(ec);

    return false;
}

typename session::object_key 
session::subscribe_stop(notify_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");
    const auto key = create_key();
    stop_subscriber_.subscribe(std::move(handler), key);
    return key;
}

bool session::notify(object_key key) NOEXCEPT
{
    return stop_subscriber_.notify_one(key, error::success);
}

void session::unsubscribe_close() NOEXCEPT
{
    network_.unsubscribe_close(identifier_);
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

channel::ptr session::create_channel(const socket::ptr& socket,
    bool quiet) NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    // Channel id must be created using create_key().
    const auto id = create_key();
    return std::make_shared<channel>(log(), socket, settings(), id, quiet);
}

// At one object/session/ns, this overflows in ~585 years (and handled).
session::object_key session::create_key() NOEXCEPT
{
    BC_ASSERT_MSG(network_.stranded(), "strand");

    if (is_zero(++keys_))
    {
        BC_ASSERT_MSG(false, "overflow");
        LOGF("Session object overflow.");
    }

    return keys_;
}

// Properties.
// ----------------------------------------------------------------------------

bool session::stopped() const NOEXCEPT
{
    return stopped_.load();
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

const network::settings& session::settings() const NOEXCEPT
{
    return network_.network_settings();
}

uint64_t session::identifier() const NOEXCEPT
{
    return identifier_;
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

asio::strand& session::strand() NOEXCEPT
{
    return network_.strand();
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
