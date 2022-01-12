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
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>
#include <bitcoin/network/settings.hpp>

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
protocol_version_31402::protocol_version_31402(channel::ptr channel, p2p& network)
  : protocol_version_31402(channel, network,
        network.network_settings().protocol_maximum,
        network.network_settings().services,
        network.network_settings().invalid_services,
        network.network_settings().protocol_minimum,
        messages::service::node_none
        /*network.network_settings().services*/)
{
}

protocol_version_31402::protocol_version_31402(channel::ptr channel,
    p2p& network, uint32_t own_version, uint64_t own_services,
    uint64_t invalid_services, uint32_t minimum_version,
    uint64_t minimum_services)
  : protocol_timer(channel, network.network_settings().channel_handshake(),
      false),
    network_(network),
    own_version_(own_version),
    own_services_(own_services),
    invalid_services_(invalid_services),
    minimum_version_(minimum_version),
    minimum_services_(minimum_services),
    CONSTRUCT_TRACK(protocol_version_31402)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_version_31402::start(event_handler /*handle_event*/)
{
    // TODO: just use a state member variable, this is stranded.

    ////// TODO: add custom handle_send here and make this 3 events (see seed).
    ////const auto join_handler = synchronize(handle_event, 2, "TODO",
    ////    synchronizer_terminate::on_error);

    ////// TIMER/EVENTS START COMPLETES WITHOUT INVOKING THE HANDLER.
    ////// protocol_events retains join_handler to be invoked multiple times.
    ////// handle_event is invoked on the channel thread.
    ////protocol_timer::start(join_handler);

    SUBSCRIBE2(version, handle_receive_version, _1, _2);
    SUBSCRIBE2(version_acknowledge, handle_receive_version_acknowledge, _1, _2);
    SEND2(version_factory(), handle_send, _1, version::command);
}

messages::version protocol_version_31402::version_factory() const
{
    const auto timestamp = static_cast<uint32_t>(zulu_time());
    const auto& settings = network_.network_settings();
    const auto height = network_.top_block().height();

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
            authority().ip(),
            authority().port(),
        },
        {
            // We always match the services declared in our version.services.
            timestamp,
            own_services_,
            settings.self.ip(),
            settings.self.port(),
        },
        nonce(),
        BC_USER_AGENT,
        static_cast<uint32_t>(height),
        false
    };
}

// Protocol.
// ----------------------------------------------------------------------------

bool protocol_version_31402::handle_receive_version(const code& ec,
    version::ptr message)
{
    if (stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving version from [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return false;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Peer [" << authority() << "] protocol version ("
        << message->value << ") user agent: " << message->user_agent;

    // TODO: move these three checks to initialization.
    //-------------------------------------------------------------------------

    const auto& settings = network_.network_settings();

    if (settings.protocol_minimum < level::minimum_protocol)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, minimum below ("
            << level::minimum_protocol << ").";
        set_event(error::channel_stopped);
        return false;
    }

    if (settings.protocol_maximum > level::maximum_protocol)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, maximum above ("
            << level::maximum_protocol << ").";
        set_event(error::channel_stopped);
        return false;
    }

    if (settings.protocol_minimum > settings.protocol_maximum)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, "
            << "minimum exceeds maximum.";
        set_event(error::channel_stopped);
        return false;
    }

    //-------------------------------------------------------------------------

    if (!sufficient_peer(message))
    {
        set_event(error::channel_stopped);
        return false;
    }

    const auto version = std::min(message->value, own_version_);
    set_negotiated_version(version);
    set_peer_version(message);

    LOG_DEBUG(LOG_NETWORK)
        << "Negotiated protocol version (" << version
        << ") for [" << authority() << "]";

    SEND2(version_acknowledge{}, handle_send, _1, version_acknowledge::command);

    // 1 of 2
    set_event(error::success);
    return false;
}

bool protocol_version_31402::sufficient_peer(version::ptr message)
{
    if ((message->services & invalid_services_) != 0)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Invalid peer network services (" << message->services
            << ") for [" << authority() << "]";
        return false;
    }

    if ((message->services & minimum_services_) != minimum_services_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer network services (" << message->services
            << ") for [" << authority() << "]";
        return false;
    }

    if (message->value < minimum_version_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer protocol version (" << message->value
            << ") for [" << authority() << "]";
        return false;
    }

    return true;
}

bool protocol_version_31402::handle_receive_version_acknowledge(const code& ec,
    version_acknowledge::ptr)
{
    if (stopped(ec))
        return false;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving verack from [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return false;
    }

    // 2 of 2
    set_event(error::success);
    return false;
}

const std::string& protocol_version_31402::name() const
{
    return protocol_name;
}

} // namespace network
} // namespace libbitcoin
