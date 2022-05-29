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
#include <bitcoin/network/protocols/protocol_version_31402.hpp>

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_version_31402
static const std::string protocol_name = "version";

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// Require the configured minimum protocol and services by default.
protocol_version_31402::protocol_version_31402(const session& session,
    const channel::ptr& channel) noexcept
  : protocol_version_31402(session, channel,
      session.settings().services_minimum,
      session.settings().services_maximum)
{
}

// Used for seeding (should probably not override these).
protocol_version_31402::protocol_version_31402(const session& session,
    const channel::ptr& channel, uint64_t minimum_services,
    uint64_t maximum_services) noexcept
  : protocol(session, channel),
    minimum_version_(session.settings().protocol_minimum),
    maximum_version_(session.settings().protocol_maximum),
    minimum_services_(minimum_services),
    maximum_services_(maximum_services),
    invalid_services_(session.settings().invalid_services),
    sent_version_(false),
    received_version_(false),
    received_acknowledge_(false),
    timer_(std::make_shared<deadline>(channel->strand(),
        session.settings().channel_handshake()))
{
}

const std::string& protocol_version_31402::name() const noexcept
{
    return protocol_name;
}

// Utilities.
// ----------------------------------------------------------------------------

// Allow derived classes to modify the version message.
protocol_version_31402::version_ptr
protocol_version_31402::version_factory() const noexcept
{
    // TODO: allow for node to inject top height.
    const auto top_height = static_cast<uint32_t>(zero);
    BC_ASSERT_MSG(top_height <= max_uint32, "Time to upgrade the protocol.");

    // Relay always exposed on version, despite lack of definition < BIP37.
    // See comments in version::deserialize regarding BIP37 protocol bug.
    constexpr auto relay = false;
    const auto timestamp = static_cast<uint32_t>(zulu_time());

    return std::make_shared<version>(
        version
        {
            maximum_version_,
            maximum_services_,
            timestamp,

            // ********************************************************************
            // PROTOCOL:
            // Peer address_item (timestamp/services are redundant/unused).
            // Both peers cannot know each other's service level, so set node_none.
            // ********************************************************************
            {
                timestamp,
                service::node_none,
                authority().to_ip_address(),
                authority().port(),
            },

            // ********************************************************************
            // PROTOCOL:
            // Self address_item (timestamp/services are redundant).
            // The protocol expects duplication of the sender's services, but this
            // is broadly observed to be inconsistently implemented by other nodes.
            // ********************************************************************
            {
                timestamp,
                maximum_services_,
                settings().self.to_ip_address(),
                settings().self.port(),
            },

            nonce(),
            BC_USER_AGENT,
            top_height,
            relay
        });
}

// Allow derived classes to handle message rejection.
void protocol_version_31402::rejection(const code& ec) noexcept
{
    callback(ec);
}

// Start/Stop.
// ----------------------------------------------------------------------------

// Session resumes the channel following return from start().
// Sends are not precluded, but no messages can be received while paused.
void protocol_version_31402::shake(result_handler&& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");

    if (started())
    {
        handler(error::operation_failed);
        return;
    }

    handler_ = std::make_shared<result_handler>(std::move(handler));

    if (minimum_version_ < level::minimum_protocol)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, minimum below ("
            << level::minimum_protocol << ")." << std::endl;

        callback(error::invalid_configuration);
        return;
    }

    if (maximum_version_ > level::maximum_protocol)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, maximum above ("
            << level::maximum_protocol << ")." << std::endl;

        callback(error::invalid_configuration);
        return;
    }

    if (minimum_version_ > maximum_version_)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, "
            << "minimum exceeds maximum." << std::endl;

        callback(error::invalid_configuration);
        return;
    }

    SUBSCRIBE2(version, handle_receive_version, _1, _2);
    SUBSCRIBE2(version_acknowledge, handle_receive_acknowledge, _1, _2);
    SEND1(std::move(*version_factory()), handle_send_version, _1);

    protocol::start();
}

// Allow service shutdown to terminate handshake.
void protocol_version_31402::stopping(const code& ec) noexcept
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");
    callback(ec);
}

bool protocol_version_31402::complete() const noexcept
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");
    return sent_version_ && received_version_ && received_acknowledge_;
}

// Idempotent on the strand, first caller gets handler.
void protocol_version_31402::callback(const code& ec) noexcept
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");

    // This will asynchronously invoke handle_timer and if the channel is not
    // stopped, will then invoke callback(error::operation_canceled).
    timer_->stop();

    if (!handler_)
        return;

    // There may be a post-handshake message already waiting on the socket.
    // The channel must be paused while still on the channel strand to prevent
    // acceptance until after protocol attachment (and resume). So session will
    // pause the channel within this handler.
    (*handler_)(ec);
    handler_.reset();
}

void protocol_version_31402::handle_timer(const code& ec) noexcept
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");

    if (stopped())
        return;

    // error::operation_canceled is set when stopped (caught above), but will
    // also be set upon successful completion, as this also cancels the timer.
    // However in this case the code will be ignored in the completion handler.
    if (ec)
    {
        callback(ec);
        return;
    }

    callback(error::channel_timeout);
}

// Outgoing [send_version... receive_acknowledge].
// ----------------------------------------------------------------------------

void protocol_version_31402::handle_send_version(const code& ec) noexcept
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");

    if (stopped(ec))
        return;

    timer_->start(BIND1(handle_timer, _1));
    sent_version_ = true;

    if (complete())
        callback(error::success);
}

void protocol_version_31402::handle_receive_acknowledge(const code& ec,
    const version_acknowledge::ptr&) noexcept
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");

    if (stopped(ec))
        return;

    // Premature or multiple verack disallowed (persists for channel life).
    if (!sent_version_ || received_acknowledge_)
    {
        rejection(error::protocol_violation);
        return;
    }

    received_acknowledge_ = true;

    // Ensure that no message is read after two required.
    // The reader is suspended within this handler by the strand.
    if (received_version_)
        pause();

    if (complete())
        callback(error::success);
}

// Incoming [receive_version => send_acknowledge].
// ----------------------------------------------------------------------------

void protocol_version_31402::handle_receive_version(const code& ec,
    const version::ptr& message) noexcept
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");

    if (stopped(ec))
        return;

    // Multiple version messages not allowed (persists for channel life).
    if (received_version_)
    {
        rejection(error::protocol_violation);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Peer [" << authority() << "] protocol version ("
        << message->value << ") user agent: " << message->user_agent
        << std::endl;

    if (to_bool(message->services & invalid_services_))
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Invalid peer network services (" << message->services
            << ") for [" << authority() << "]" << std::endl;

        rejection(error::insufficient_peer);
        return;
    }

    // Advertised services on many incoming connections may be set to zero.
    if ((message->services & minimum_services_) != minimum_services_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer network services (" << message->services
            << ") for [" << authority() << "]" << std::endl;

        rejection(error::insufficient_peer);
        return;
    }

    if (message->value < minimum_version_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer protocol version (" << message->value
            << ") for [" << authority() << "]" << std::endl;

        rejection(error::insufficient_peer);
        return;
    }

    // TODO: * denotes unversioned protocol.
    // TODO: Versioned protocol classes are suffixed as: x_version[_sub].
    // TODO: Unversioned protocol classes are suffixed as: x_unversioned[_sub].
    // TODO: Handle unversioned handhshake PIDs in base: version_unversioned.

    // TODO: Get own PIDs from settings ([protocol].bipXXX).
    // TODO: These augment protocol minimum version levels.
    // TODO: Own PID values are const (relay overriden in seeding).
    // TODO: Could set version < bip37 for seeding, allows sendaddrv2, though
    // TODO: this would require making certain assumptions about peer support.

    // TODO: sendrecon is two PIDs for both own/peer (*sendrecon_in/out[1]).
    // TODO: relay is one PID for both own/peer (all PIDs have own/peer).
    // TODO: Nodes with bip133 version level do not have to implement bip133.
    // TODO: Peer may not send disabletx if our relay PID is true (bip338).
    // TODO: The disabletx and relay PIDs are independent (bip338).
    // TODO: Drop peer for send of message if its state is already set.

    // TODO: PID handshake messages may also be caught by their protocols but
    // TODO: handshake PID state must only be updated by the version protocols.
    // TODO: This is necessary only to obtain draft:sendrecon.salt (bip330).

    // TODO: Send own relay PID in version written to version.relay.
    // TODO: Send own handshake PIDs in version as handshake messages:
    // TODO:    *sendaddrv2[], wtxidrelay[], disabletx[]
    // TODO: Send own post-handshake PIDs in protocols.
    // TODO:    sendheaders[], sendcmpct[0|1],
    // TODO:    (draft: *sendrecon_in[1], *sendrecon_out[1])

    // TODO: Set peer relay PID in version read from version.relay.
    // TODO: Set peer handshake PIDs in version from handshake messages:
    // TODO:    *sendaddrv2[], wtxidrelay[], disabletx[]
    // TODO: Set peer post-handshake PIDs in protocols:
    // TODO:    sendheaders[], sendcmpct[0|1],
    // TODO:    (draft: *sendrecon_in[1], *sendrecon_out[1])

    // TODO: Compute negotiated PIDs.
    // TODO: PID protocols are independently own/peer.
    // TODO: Versioned PID protocols are opt in (assured).
    // TODO: Unversioned PID protocols are opt in (maybe).
    // TODO: Dynamic computation is only necessary for dynamic PID protocols:
    //          sendheaders, sendcmpct, (draft: sendrecon)

    const auto version = std::min(message->value, maximum_version_);
    set_negotiated_version(version);
    set_peer_version(message);

    LOG_DEBUG(LOG_NETWORK)
        << "Negotiated protocol version (" << version
        << ") for [" << authority() << "]" << std::endl;

    SEND1(version_acknowledge{}, handle_send_acknowledge, _1);

    // Handle in handle_send_acknowledge.
    received_version_ = true;

    // Ensure that no message is read after two required.
    // The reader is suspended within this handler by the strand.
    if (received_acknowledge_)
        pause();
}

void protocol_version_31402::handle_send_acknowledge(const code& ec) noexcept
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");

    if (stopped(ec))
        return;

    if (complete())
        callback(error::success);
}

} // namespace network
} // namespace libbitcoin
