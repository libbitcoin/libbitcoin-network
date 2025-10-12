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
#include <bitcoin/network/sessions/session_inbound.hpp>

#include <utility>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/sessions/session_peer.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_inbound

using namespace system;
using namespace std::placeholders;

// Bind throws (ok).
// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_inbound::session_inbound(net& network, uint64_t identifier) NOEXCEPT
  : session_peer(network, identifier), tracker<session_inbound>(network.log)
{
}

// Start/stop sequence.
// ----------------------------------------------------------------------------

void session_inbound::start(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (!settings().inbound_enabled())
    {
        LOGN("Not configured for inbound peer connections.");
        handler(error::success);
        unsubscribe_close();
        return;
    }

    session_peer::start(BIND(handle_started, _1, std::move(handler)));
}

void session_inbound::handle_started(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!stopped(), "session stopped in start");

    if (ec)
    {
        handler(ec);
        unsubscribe_close();
        return;
    }

    LOGN("Accepting " << settings().inbound_connections << " peers on "
        << settings().binds.size() << " bindings.");

    for (const auto& bind: settings().binds)
    {
        const auto acceptor = create_acceptor();

        // Require that all acceptors at least start.
        if (const auto error_code = acceptor->start(bind))
        {
            handler(error_code);
            return;
        }

        LOGN("Bound to peer endpoint [" << acceptor->local() << "].");

        // Subscribe acceptor to stop desubscriber.
        subscribe_stop([=](const code&) NOEXCEPT
        {
            acceptor->stop();
            return false;
        });

        start_accept(error::success, acceptor);
    }

    handler(error::success);
}

// Accept cycle.
// ----------------------------------------------------------------------------

// Attempt to accept peers on each configured endpoint.
void session_inbound::start_accept(const code&,
    const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates accept loop (and acceptor is restartable).
    if (stopped())
        return;

    acceptor->accept(BIND(handle_accepted, _1, _2, acceptor));
}

void session_inbound::handle_accepted(const code& ec,
    const socket::ptr& socket, const acceptor::ptr& acceptor) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Guard restartable timer (shutdown delay).
    if (stopped())
    {
        if (socket) socket->stop();
        return;
    }

    if (ec == error::service_suspended)
    {
        ////LOGS("Suspended inbound channel start.");
        defer(BIND(start_accept, _1, acceptor));
        return;
    }

    // There was an error accepting the channel, so try again after delay.
    if (ec)
    {
        BC_ASSERT_MSG(!socket || socket->stopped(), "unexpected socket");
        LOGF("Failed to accept inbound peer connection, " << ec.message());
        defer(BIND(start_accept, _1, acceptor));
        return;
    }

    if (!enabled())
    {
        LOGS("Dropping inbound peer connection (disabled).");
        socket->stop();
        return;
    }

    // Could instead stop listening when at limit, though this is simpler.
    if (inbound_channel_count() >= settings().inbound_connections)
    {
        LOGS("Dropping oversubscribed peer [" << socket->authority() << "].");
        socket->stop();
        return;
    }

    const auto address = socket->authority().to_address_item();

    if (!whitelisted(address))
    {
        ////LOGS("Dropping not whitelisted peer [" << socket->authority() << "].");
        socket->stop();
        return;
    }

    if (blacklisted(address))
    {
        ////LOGS("Dropping blacklisted peer [" << socket->authority() << "].");
        socket->stop();
        return;
    }

    const auto channel = create_channel(socket);

    LOGS("Accepted peer connection [" << channel->authority()
        << "] on binding [" << acceptor->local() << "].");

    // There was no error, so listen again without delay.
    start_accept(error::success, acceptor);

    start_channel(channel,
        BIND(handle_channel_start, _1, channel),
        BIND(handle_channel_stop, _1, channel));
}

bool session_inbound::blacklisted(const config::address& address) const NOEXCEPT
{
    return settings().blacklisted(address);
}

bool session_inbound::whitelisted(const config::address& address) const NOEXCEPT
{
    return settings().whitelisted(address);
}

bool session_inbound::enabled() const NOEXCEPT
{
    return true;
}

// Completion sequence.
// ----------------------------------------------------------------------------

void session_inbound::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for attach");

    // Inbound does not require any node services.
    using namespace messages::peer;
    constexpr auto minimum_services = service::node_none;
    const auto maximum_services = settings().services_maximum;

    // Protocol must pause the channel after receiving version and verack.
    const auto self = shared_from_this();
    const auto relay = settings().enable_relay;
    const auto reject = settings().enable_reject;
    const auto address_v2 = settings().enable_address_v2;

    // protocol_version_70016 sends and receives send_address_v2 even though
    // inbound connections do not accept addresses. There is no message to
    // disable address broadcasting, so this is just allowed to upgrade.

    // Address v2 can be disabled, independent of version.
    if (is_configured(level::bip155) && address_v2)
        channel->attach<protocol_version_70016>(self, minimum_services,
            maximum_services, relay, reject)->shake(std::move(handler));

    // Protocol versions are cumulative, but reject is deprecated.
    else if (is_configured(level::bip61) && reject)
        channel->attach<protocol_version_70002>(self, minimum_services,
            maximum_services, relay)->shake(std::move(handler));

    // TODO: consider relay may be dynamic (disabled until current).
    // settings().enable_relay is always passed to the peer during handshake.
    else if (is_configured(level::bip37))
        channel->attach<protocol_version_70001>(self, minimum_services,
            maximum_services, relay)->shake(std::move(handler));

    else if (is_configured(level::version_message))
        channel->attach<protocol_version_106>(self, minimum_services,
            maximum_services)->shake(std::move(handler));
}

void session_inbound::handle_channel_start(const code&,
    const channel::ptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    ////LOGS("Inbound channel start [" << channel->authority() << "] "
    ////    << ec.message());
}

void session_inbound::attach_protocols(
    const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for protocol attach");

    using namespace messages::peer;
    const auto self = shared_from_this();
    const auto peer = std::dynamic_pointer_cast<channel_peer>(channel);

    // Alert is deprecated, independent of version.
    if (peer->is_negotiated(level::alert_message) && settings().enable_alert)
        channel->attach<protocol_alert_311>(self)->start();

    // Reject is deprecated, independent of version.
    if (peer->is_negotiated(level::bip61) && settings().enable_reject)
        channel->attach<protocol_reject_70002>(self)->start();

    if (peer->is_negotiated(level::bip31))
        channel->attach<protocol_ping_60001>(self)->start();
    else if (peer->is_negotiated(level::version_message))
        channel->attach<protocol_ping_106>(self)->start();

    // Attach is overridden to disable inbound address protocols.

    if (settings().enable_address_v2)
    {
        ////// Sending address v2 is enabled in handshake.
        ////if (peer->send_address_v2())
        ////    channel->attach<protocol_address_out_70016>(self)->start();
    }

    if (settings().enable_address)
    {
        if (peer->is_negotiated(level::get_address_message))
            channel->attach<protocol_address_out_209>(self)->start();
        ////else if (peer->is_negotiated(level::version_message))
        ////    channel->attach<protocol_address_out_106>(self)->start();
    }
}

void session_inbound::handle_channel_stop(const code& LOG_ONLY(ec),
    const channel::ptr& LOG_ONLY(channel)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    LOGS("Inbound peer channel stop [" << channel->authority() << "] "
        << ec.message());
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
