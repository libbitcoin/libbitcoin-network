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
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

#define CLASS protocol_version_31402
static const std::string protocol_name = "version";

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// Require the configured minimum protocol and services by default.
protocol_version_31402::protocol_version_31402(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol_version_31402(session, channel,
      session.settings().services_minimum,
      session.settings().services_maximum)
{
}

// Used for seeding (should probably not override these).
protocol_version_31402::protocol_version_31402(const session& session,
    const channel::ptr& channel, uint64_t minimum_services,
    uint64_t maximum_services) NOEXCEPT
  : protocol(session, channel),
    minimum_version_(session.settings().protocol_minimum),
    maximum_version_(session.settings().protocol_maximum),
    minimum_services_(minimum_services),
    maximum_services_(maximum_services),
    invalid_services_(session.settings().invalid_services),
    sent_version_(false),
    received_version_(false),
    received_acknowledge_(false),
    timer_(std::make_shared<deadline>(session.log(), channel->strand(),
        session.settings().channel_handshake())),
    tracker<protocol_version_31402>(session.log())
{
}

const std::string& protocol_version_31402::name() const NOEXCEPT
{
    return protocol_name;
}

// Utilities.
// ----------------------------------------------------------------------------

// Allow derived classes to modify the version message.
// Relay always exposed on version, despite lack of definition < BIP37.
// See comments in version::deserialize regarding BIP37 protocol bug.
messages::version protocol_version_31402::version_factory(
    bool relay) const NOEXCEPT
{
    // TODO: allow for node to inject top height.
    const auto timestamp = unix_time();
    constexpr auto top_height = possible_narrow_cast<uint32_t>(zero);
    BC_ASSERT_MSG(top_height <= max_uint32, "Time to upgrade the protocol.");

    // Should construct using makes_shared(vargs) overload, but fails on clang.
    BC_PUSH_WARNING(NO_NEW_OR_DELETE)
    return
    {
        maximum_version_,
        maximum_services_,
        timestamp,

        // ********************************************************************
        // PROTOCOL:
        // Peer address_item (timestamp/services are redundant/unused).
        // Both peers cannot know each other's service level, so set node_none.
        // ********************************************************************
        address_item
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
        // Peers may provide a null address for self, commonly observed.
        // ********************************************************************
        address_item
        {
            timestamp,
            maximum_services_,
            settings().self.to_ip_address(),
            settings().self.port(),
        },

        nonce(),
        settings().user_agent,
        top_height,
        relay
    };
    BC_POP_WARNING()
}

// Allow derived classes to handle message rejection.
void protocol_version_31402::rejection(const code& ec) NOEXCEPT
{
    callback(ec);
}

// Start/Stop.
// ----------------------------------------------------------------------------

// Session resumes the channel following return from start().
// Sends are not precluded, but no messages can be received while paused.
void protocol_version_31402::shake(result_handler&& handler) NOEXCEPT
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
        LOG("Invalid protocol version configuration, minimum below ("
            << level::minimum_protocol << ").");

        callback(error::invalid_configuration);
        return;
    }

    if (maximum_version_ > level::maximum_protocol)
    {
        LOG("Invalid protocol version configuration, maximum above ("
            << level::maximum_protocol << ").");

        callback(error::invalid_configuration);
        return;
    }

    if (minimum_version_ > maximum_version_)
    {
        LOG("Invalid protocol version configuration, minimum above maximum.");
        callback(error::invalid_configuration);
        return;
    }

    SUBSCRIBE2(version, handle_receive_version, _1, _2);
    SUBSCRIBE2(version_acknowledge, handle_receive_acknowledge, _1, _2);
    SEND1(version_factory(), handle_send_version, _1);

    protocol::start();
}

// Allow service shutdown to terminate handshake.
void protocol_version_31402::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");
    callback(ec);
}

bool protocol_version_31402::complete() const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");
    return sent_version_ && received_version_ && received_acknowledge_;
}

// Idempotent on the strand, first caller gets handler.
void protocol_version_31402::callback(const code& ec) NOEXCEPT
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

void protocol_version_31402::handle_timer(const code& ec) NOEXCEPT
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

void protocol_version_31402::handle_send_version(const code& ec) NOEXCEPT
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
    const version_acknowledge::cptr&) NOEXCEPT
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
    const version::cptr& message) NOEXCEPT
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

    LOG("Peer [" << authority() << "] protocol version ("
        << message->value << ") user agent: " << message->user_agent);

    if (to_bool(message->services & invalid_services_))
    {
        LOG("Invalid peer network services (" << message->services
            << ") for [" << authority() << "]");

        rejection(error::insufficient_peer);
        return;
    }

    // Advertised services on many incoming connections are set to zero.
    if ((message->services & minimum_services_) != minimum_services_)
    {
        LOG("Insufficient peer network services (" << message->services << ") "
            "for [" << authority() << "]");

        rejection(error::insufficient_peer);
        return;
    }

    if (message->value < minimum_version_)
    {
        LOG("Insufficient peer protocol version (" << message->value << ") "
            "for [" << authority() << "]");

        rejection(error::insufficient_peer);
        return;
    }

    const auto version = std::min(message->value, maximum_version_);
    set_negotiated_version(version);
    set_peer_version(message);

    LOG("Negotiated protocol version (" << version << ") "
        << "for [" << authority() << "] ");

    // TODO: verbose (helpful for identifying own address for config of self).
    LOG("Peer [" << authority() << "] "
        << "as {" << config::authority(message->address_sender) << "} "
        << "us {" << config::authority(message->address_receiver) << "}");

    SEND1(version_acknowledge{}, handle_send_acknowledge, _1);

    // Handle in handle_send_acknowledge.
    received_version_ = true;

    // Ensure that no message is read after two required.
    // The reader is suspended within this handler by the strand.
    if (received_acknowledge_)
        pause();
}

void protocol_version_31402::handle_send_acknowledge(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_31402");

    if (stopped(ec))
        return;

    if (complete())
        callback(error::success);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
