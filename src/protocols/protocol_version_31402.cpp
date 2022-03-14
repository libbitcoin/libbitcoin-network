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

#include <cstdint>
#include <functional>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_version_31402
static const std::string protocol_name = "version";

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// TODO: set explicitly on inbound (none or new config) and self on outbound.
// Require the configured minimum and services by default.
// Configured min version is our own but we may require higer for some stuff.
// Configured services was our but we found that most incoming connections are
// set to zero, so that is currently the default (see below).
protocol_version_31402::protocol_version_31402(const session& session,
    channel::ptr channel)
  : protocol_version_31402(session, channel,
        session.settings().protocol_maximum,
        session.settings().services,
        session.settings().invalid_services,
        session.settings().protocol_minimum,
        messages::service::node_none
        /*session.settings().services*/)
{
}

protocol_version_31402::protocol_version_31402(const session& session,
    channel::ptr channel,
    uint32_t own_version, uint64_t own_services,
    uint64_t invalid_services, uint32_t minimum_version,
    uint64_t minimum_services)
  : protocol_timer(session, channel, session.settings().channel_handshake(), false),
    own_version_(own_version),
    own_services_(own_services),
    invalid_services_(invalid_services),
    minimum_version_(minimum_version),
    minimum_services_(minimum_services)
{
}

// Start sequence.
// ----------------------------------------------------------------------------


// handle_event must be invoked upon completion or failure.
void protocol_version_31402::start(result_handler handle_event)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    // TODO: just use a state member variable, this is stranded.

    ////// TODO: add custom handle_send here and make this 3 events (see seed).
    ////const auto join_handler = synchronize(handle_event, 2, "TODO",
    ////    synchronizer_terminate::on_error);

    ////// protocol_events retains join_handler to be invoked multiple times.
    ////// handle_event is invoked on the channel thread.
    ////protocol_timer::start(join_handler);

    SUBSCRIBE2(version, handle_receive_version, _1, _2);
    SUBSCRIBE2(version_acknowledge, handle_receive_acknowledge, _1, _2);
    SEND2(version_factory(), handle_send, _1, version::command);
}

// TODO: allow for node to inject top height.
messages::version protocol_version_31402::version_factory() const
{
    BC_ASSERT_MSG(stranded(), "stranded");

    const auto timestamp = static_cast<uint32_t>(zulu_time());
    const auto height = zero;//// network_.top_block().height();

    BC_ASSERT_MSG(height <= max_uint32, "Time to upgrade the protocol.");

    return
    {
        own_version_,
        own_services_,
        timestamp,
        {
            // The peer's services cannot be reflected, so zero it.
            timestamp,
            service::node_none,
            authority().to_ip_address(),
            authority().port(),
        },
        {
            // We always match the services declared in our version.services.
            timestamp,
            own_services_,
            settings().self.to_ip_address(),
            settings().self.port(),
        },
        nonce(),
        BC_USER_AGENT,
        static_cast<uint32_t>(height),
        false
    };
}

// Protocol.
// ----------------------------------------------------------------------------

void protocol_version_31402::handle_receive_version(const code& ec,
    version::ptr message)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving version from [" << authority() << "] "
            << ec.message() << std::endl;
        set_event(ec);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Peer [" << authority() << "] protocol version ("
        << message->value << ") user agent: " << message->user_agent
        << std::endl;

    // TODO: move these three checks to initialization.
    //-------------------------------------------------------------------------

    if (settings().protocol_minimum < level::minimum_protocol)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, minimum below ("
            << level::minimum_protocol << ")." << std::endl;
        set_event(error::channel_stopped);
        return;
    }

    if (settings().protocol_maximum > level::maximum_protocol)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, maximum above ("
            << level::maximum_protocol << ")." << std::endl;
        set_event(error::channel_stopped);
        return;
    }

    if (settings().protocol_minimum > settings().protocol_maximum)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, "
            << "minimum exceeds maximum." << std::endl;
        set_event(error::channel_stopped);
        return;
    }

    //-------------------------------------------------------------------------

    if (!sufficient_peer(message))
    {
        set_event(error::channel_stopped);
        return;
    }

    const auto version = std::min(message->value, own_version_);
    set_negotiated_version(version);
    set_peer_version(message);

    LOG_DEBUG(LOG_NETWORK)
        << "Negotiated protocol version (" << version
        << ") for [" << authority() << "]" << std::endl;

    SEND2(version_acknowledge{}, handle_send, _1, version_acknowledge::command);

    // 1 of 2
    set_event(error::success);
}

bool protocol_version_31402::sufficient_peer(version::ptr message)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (to_bool(message->services & invalid_services_))
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Invalid peer network services (" << message->services
            << ") for [" << authority() << "]" << std::endl;
        return false;
    }

    if ((message->services & minimum_services_) != minimum_services_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer network services (" << message->services
            << ") for [" << authority() << "]" << std::endl;
        return false;
    }

    if (message->value < minimum_version_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer protocol version (" << message->value
            << ") for [" << authority() << "]" << std::endl;
        return false;
    }

    return true;
}

void protocol_version_31402::handle_receive_acknowledge(const code& ec,
    version_acknowledge::ptr)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving verack from [" << authority() << "] "
            << ec.message() << std::endl;
        set_event(ec);
        return;
    }

    // 2 of 2
    set_event(error::success);
}

const std::string& protocol_version_31402::name() const
{
    return protocol_name;
}

} // namespace network
} // namespace libbitcoin
