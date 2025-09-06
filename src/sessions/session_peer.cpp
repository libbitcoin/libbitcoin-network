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
#include <bitcoin/network/sessions/session_peer.hpp>

#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/net.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

#define CLASS session_peer

using namespace system;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

session_peer::session_peer(net& network, uint64_t identifier) NOEXCEPT
  : network_(network), session(network, identifier)
{
}

// Channel sequence.
// ----------------------------------------------------------------------------

void session_peer::start_channel(const channel::ptr& channel,
    result_handler&& starter, result_handler&& stopper) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // In case of a loopback, inbound and outbound are on the same strand.
    // Inbound does not check nonce until handshake completes, so no race.
    if (!stopped())
    {
        const auto peer = std::dynamic_pointer_cast<channel_peer>(channel);
        if (!network_.store_nonce(*peer))
        {
            channel->stop(error::channel_conflict);
            starter(error::channel_conflict);
            stopper(error::channel_conflict);
            return;
        }
    }

    session::start_channel(channel, std::move(starter), std::move(stopper));
}

void session_peer::do_handle_handshake(const code& ec, const channel::ptr& channel,
    const result_handler& start) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Handles channel and protocol start failures.
    const auto peer = std::dynamic_pointer_cast<channel_peer>(channel);
    const auto code = ec ? ec : network_.count_channel(*peer);

    if (code)
    {
        unpend(channel);
        network_.unstore_nonce(*peer);
        channel->stop(code);
        start(code);
        return;
    }

    // Requires uncount_channel/unstore_nonce on stop if and only if success.
    start(ec);
}

void session_peer::do_attach_protocols(const channel::ptr& channel,
    const result_handler& started) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for protocol attach");

    // Protocol attach is always synchronous, complete here.
    attach_protocols(channel);

    // Notify channel subscribers of fully-attached non-seed channel.
    const auto peer = std::dynamic_pointer_cast<channel_peer>(channel);
    if (!peer->quiet())
        network_.notify_connect(channel);

    // Resume accepting messages on the channel, timers restarted.
    channel->resume();

    // Complete on network strand.
    boost::asio::post(network_.strand(),
        std::bind(started, error::success));
}

// Unnonce in stop vs. handshake to avoid loopback race (in/out same strand).
void session_peer::do_handle_channel_stopped(const code& ec,
    const channel::ptr& channel, const result_handler& stopped) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    const auto peer = std::dynamic_pointer_cast<channel_peer>(channel);
    unpend(channel);
    network_.unstore_nonce(*peer);
    network_.uncount_channel(*peer);
    unsubscribe(channel->identifier());

    // Assume stop notification, but may be subscribe failure (idempotent).
    // Handles stop reason code, stop subscribe failure or stop notification.
    stopped(ec);
}

// Channel sequence.
// ----------------------------------------------------------------------------

void session_peer::attach_handshake(const channel::ptr& channel,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for handshake attach");

    // Protocol must pause the channel after receiving version and verack.
    using namespace messages;
    const auto self = shared_from_this();

    // Address v2 can be disabled, independent of version.
    if (is_configured(level::bip155) && settings().enable_address_v2)
        channel->attach<protocol_version_70016>(self)->shake(std::move(handler));

    // Protocol versions are cumulative, but reject is deprecated.
    else if (is_configured(level::bip61) && settings().enable_reject)
        channel->attach<protocol_version_70002>(self)->shake(std::move(handler));

    // TODO: consider relay may be dynamic (disabled until current).
    // settings().enable_relay is always passed to the peer during handshake.
    else if (is_configured(level::bip37))
        channel->attach<protocol_version_70001>(self)->shake(std::move(handler));

    else if (is_configured(level::version_message))
        channel->attach<protocol_version_106>(self)->shake(std::move(handler));
}

// Override in derived sessions to attach protocols.
void session_peer::attach_protocols(const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for protocol attach");

    using namespace messages;
    const auto self = shared_from_this();
    const auto peer = std::dynamic_pointer_cast<channel_peer>(channel);

    // Alert is deprecated, independent of version.
    if (peer->is_negotiated(level::alert_message) && settings().enable_alert)
        channel->attach<protocol_alert_311>(self)->start();

    // Reject is deprecated, independent of version.
    if (peer->is_negotiated(level::bip61) && settings().enable_reject)
        channel->attach<protocol_reject_70002>(self)->start();

    if (peer->is_negotiated(level::bip31))
        peer->attach<protocol_ping_60001>(self)->start();
    else if (peer->is_negotiated(level::version_message))
        channel->attach<protocol_ping_106>(self)->start();

    if (settings().enable_address_v2)
    {
        ////// Address v2 can be disabled, independent of version.
        ////if (peer->is_negotiated(level::bip155)
        ////    channel->attach<protocol_address_in_70016>(self)->start();
    
        ////// Sending address v2 is enabled in handshake.
        ////if (peer->send_address_v2())
        ////    channel->attach<protocol_address_out_70016>(self)->start();
    }

    if (settings().enable_address)
    {
        if (peer->is_negotiated(level::get_address_message))
        {
            channel->attach<protocol_address_in_209>(self)->start();
            channel->attach<protocol_address_out_209>(self)->start();
        }
        else if (peer->is_negotiated(level::version_message))
        {
            ////channel->attach<protocol_address_in_106>(self)->start();
            ////channel->attach<protocol_address_out_106>(self)->start();
        }
    }
}

// Factories.
// ----------------------------------------------------------------------------

acceptor::ptr session_peer::create_acceptor() NOEXCEPT
{
    return network_.create_acceptor();
}

connector::ptr session_peer::create_connector() NOEXCEPT
{
    return network_.create_connector();
}

connectors_ptr session_peer::create_connectors(size_t count) NOEXCEPT
{
    return network_.create_connectors(count);
}

channel::ptr session_peer::create_channel(const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Default message memory resource, override create_channel to replace.
    static default_memory memory{};

    // Channel id must be created using create_key().
    return std::make_shared<channel_peer>(memory, log, socket, settings(),
        create_key());
}

// Properties.
// ----------------------------------------------------------------------------

size_t session_peer::address_count() const NOEXCEPT
{
    return network_.address_count();
}

size_t session_peer::channel_count() const NOEXCEPT
{
    return network_.channel_count();
}

size_t session_peer::inbound_channel_count() const NOEXCEPT
{
    return network_.inbound_channel_count();
}

size_t session_peer::outbound_channel_count() const NOEXCEPT
{
    return floored_subtract(channel_count(), inbound_channel_count());
}

bool session_peer::is_configured(messages::level level) const NOEXCEPT
{
    return settings().protocol_maximum >= level;
}

// Utilities.
// ----------------------------------------------------------------------------

void session_peer::take(address_item_handler&& handler) const NOEXCEPT
{
    network_.take(std::move(handler));
}

void session_peer::fetch(address_handler&& handler) const NOEXCEPT
{
    network_.fetch(std::move(handler));
}

void session_peer::restore(const address_item_cptr& address,
    result_handler&& handler) const NOEXCEPT
{
    network_.restore(address, std::move(handler));
}

void session_peer::save(const address_cptr& message,
    count_handler&& handler) const NOEXCEPT
{
    network_.save(message, std::move(handler));
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
