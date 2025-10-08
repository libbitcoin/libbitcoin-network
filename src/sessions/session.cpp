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
#include <bitcoin/network/sessions/session.hpp>

#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/net.hpp>
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

session::session(net& network, uint64_t identifier) NOEXCEPT
  : network_(network),
    broadcaster_(network.broadcaster_),
    identifier_(identifier),
    stop_subscriber_(network.strand()),
    reporter(network.log)
{
}

session::~session() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "The session was not stopped.");
    if (!stopped()) { LOGF("~session is not stopped."); }
}

void session::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

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
    BC_ASSERT_MSG(stranded(), "strand");

    // Break out of sesson loop as handlers execute. 
    stopped_.store(true);

    // Post stop handlers to strand and clear/stop accepting subscriptions.
    // The code provides information on the reason that the channel stopped.
    stop_subscriber_.stop(error::service_stopped);
}

// Channel sequence.
// ----------------------------------------------------------------------------
// Channel and network strands share same pool, and as long as a job is
// running in the pool, it will continue to accept work. Therefore handlers
// will not be orphaned during a stop as long as they remain in the pool.

void session::start_channel(const channel::ptr& channel,
    result_handler&& starter, result_handler&& stopper) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        channel->stop(error::service_stopped);
        starter(error::service_stopped);
        stopper(error::service_stopped);
        return;
    }

    // Pend channel for connection duration (for quick stop).
    pend(channel);

    result_handler start =
        BIND(handle_channel_starting, _1, channel, std::move(starter),
            std::move(stopper));

    result_handler shake =
        BIND(handle_handshake, _1, channel, std::move(start));

    // Switch to channel context.
    // Channel/network strands share same pool.
    boost::asio::post(channel->strand(),
        BIND(do_attach_handshake, channel, std::move(shake)));
}

void session::do_attach_handshake(const channel::ptr& channel,
    const result_handler& handshake) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for handshake attach");

    attach_handshake(channel, move_copy(handshake));

    // Channel is started/paused upon creation, this begins the read loop.
    channel->resume();
}

void session::handle_handshake(const code& ec, const channel::ptr& channel,
    const result_handler& start) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");

    // Return to network context.
    boost::asio::post(network_.strand(),
        BIND(do_handle_handshake, ec, channel, start));
}

void session::do_handle_handshake(const code& ec, const channel::ptr& channel,
    const result_handler& start) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        unpend(channel);
        channel->stop(ec);
        start(ec);
        return;
    }

    // Requires uncount_channel/unstore_nonce on stop if and only if success.
    start(ec);
}

void session::handle_channel_starting(const code& ec,
    const channel::ptr& channel, const result_handler& started,
    const result_handler& stopped) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        started(ec);
        stopped(ec);
        return;
    }

    channel->subscribe_stop(
        BIND(handle_channel_stopped, _1, channel, stopped),
        BIND(handle_channel_started, _1, channel, started));
}

void session::handle_channel_started(const code& ec,
    const channel::ptr& channel, const result_handler& started) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded() || stranded(), "strand");

    // Return to network context.
    boost::asio::post(network_.strand(),
        BIND(do_handle_channel_started, ec, channel, started));
}

void session::do_handle_channel_started(const code& ec,
    const channel::ptr& channel, const result_handler& started) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Handles channel subscribe_stop code.
    if (ec)
    {
        started(ec);
        return;
    }

    // Switch to channel context (started is invoked on network strand).
    boost::asio::post(channel->strand(),
        BIND(do_attach_protocols, channel, started));
}

void session::do_attach_protocols(const channel::ptr& channel,
    const result_handler& started) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for protocol attach");

    // Protocol attach is always synchronous, complete here.
    attach_protocols(channel);

    // Resume accepting messages on the channel, timers restarted.
    channel->resume();

    // Complete on network strand.
    boost::asio::post(network_.strand(),
        std::bind(started, error::success));
}

void session::handle_channel_stopped(const code& ec,
    const channel::ptr& channel, const result_handler& stopped) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded() || stranded(), "strand");

    // Return to network context.
    boost::asio::post(network_.strand(),
        BIND(do_handle_channel_stopped, ec, channel, stopped));
}

// Unnonce in stop vs. handshake to avoid loopback race (in/out same strand).
void session::do_handle_channel_stopped(const code& ec,
    const channel::ptr& channel, const result_handler& stopped) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    unpend(channel);
    unsubscribe(channel->identifier());

    // Assume stop notification, but may be subscribe failure (idempotent).
    // Handles stop reason code, stop subscribe failure or stop notification.
    stopped(ec);
}

// Subscriptions.
// ----------------------------------------------------------------------------

void session::defer(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    defer(settings().retry_timeout(), std::move(handler));
}

void session::defer(const steady_clock::duration& timeout,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    const auto key = create_key();
    const auto timer = std::make_shared<deadline>(log, network_.strand());

    timer->start(
        BIND(handle_timer, _1, key, std::move(handler)), timeout);

    stop_subscriber_.subscribe(
        BIND(handle_defer, _1, key, timer), key);
}

void session::handle_timer(const code& ec, object_key key,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_.notify_one(key, ec);
    complete(ec);
}

bool session::handle_defer(const code&, object_key,
    const deadline::ptr& timer) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    timer->stop();
    return false;
}

void session::pend(const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_.subscribe(BIND(handle_pend, _1, channel),
        channel->identifier());
}

// Ok to not find after stop, clears before channel stop handlers fire.
void session::unpend(const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    notify(channel->identifier());
}

bool session::handle_pend(const code& ec, const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    if (ec) channel->stop(ec);
    return false;
}

typename session::object_key 
session::subscribe_stop(notify_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    const auto key = create_key();
    stop_subscriber_.subscribe(std::move(handler), key);
    return key;
}

bool session::notify(object_key key) NOEXCEPT
{
    return stop_subscriber_.notify_one(key, error::success);
}

// At one object/session/ns, this overflows in ~585 years (and handled).
session::object_key session::create_key() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    return network_.create_key();
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

// Properties.
// ----------------------------------------------------------------------------

// protected
bool session::stopped() const NOEXCEPT
{
    return stopped_.load();
}

// protected
bool session::stranded() const NOEXCEPT
{
    return network_.stranded();
}

// protected
asio::strand& session::strand() NOEXCEPT
{
    return network_.strand();
}

const network::settings& session::settings() const NOEXCEPT
{
    return network_.network_settings();
}

uint64_t session::identifier() const NOEXCEPT
{
    return identifier_;
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
