/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/protocols/protocol_version_31402.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "version"
#define CLASS protocol_version_31402

using namespace bc::message;
using namespace std::placeholders;

static const std::string reason_insufficient_services = "insufficient-services";
static const std::string reason_insufficient_version = "insufficient-version";

// TODO: move to libbitcoin utility with similar blockchain function.
static uint64_t time_stamp()
{
    // Use system clock because we require accurate time of day.
    typedef std::chrono::system_clock wall_clock;
    const auto now = wall_clock::now();
    return wall_clock::to_time_t(now);
}

// Require the configured minimum and services by default.
// Configured min version is our own but we may require higer for some stuff.
// Configured services are our own and may not always make sense to require.
protocol_version_31402::protocol_version_31402(p2p& network,
    channel::ptr channel)
  : protocol_version_31402(network, channel,
        network.network_settings().protocol_maximum,
        network.network_settings().services,
        network.network_settings().protocol_minimum,
        network.network_settings().services)
{
}

protocol_version_31402::protocol_version_31402(p2p& network,
    channel::ptr channel, uint32_t own_version, uint64_t own_services,
    uint32_t minimum_version, uint64_t minimum_services)
  : protocol_timer(network, channel, false, NAME),
    network_(network),
    own_version_(own_version),
    own_services_(own_services),
    minimum_version_(minimum_version),
    minimum_services_(minimum_services),
    CONSTRUCT_TRACK(protocol_version_31402)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_version_31402::start(event_handler handler)
{
    static const auto mode = synchronizer_terminate::on_error;
    const auto period = network_.network_settings().channel_handshake();

    // The handler is invoked in the context of the last message receipt.
    protocol_timer::start(period, synchronize(handler, 2, NAME, mode));

    SUBSCRIBE2(version, handle_receive_version, _1, _2);
    SUBSCRIBE2(verack, handle_receive_verack, _1, _2);
    SEND1(version_factory(), handle_version_sent, _1);
}

message::version protocol_version_31402::version_factory() const
{
    const auto& settings = network_.network_settings();
    const auto height = network_.top_block().height();
    BITCOIN_ASSERT_MSG(height <= max_uint32, "Time to upgrade the protocol.");

    message::version version;
    version.set_value(own_version_);
    version.set_services(own_services_);
    version.set_timestamp( time_stamp());
    version.set_address_receiver(authority().to_network_address());
    version.set_address_sender(settings.self.to_network_address());
    version.set_nonce(nonce());
    version.set_user_agent(BC_USER_AGENT);
    version.set_start_height(static_cast<uint32_t>(height));

    // The peer's services cannot be reflected, so zero it.
    version.address_receiver().set_services(version::service::none);

    // We always match the services declared in our version.services.
    version.address_sender().set_services( own_services_);
    return version;
}

// Protocol.
// ----------------------------------------------------------------------------

bool protocol_version_31402::handle_receive_version(const code& ec,
    version_const_ptr message)
{
    if (stopped())
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
        << "Peer [" << authority() << "] user agent: " << message->user_agent();

    const auto& settings = network_.network_settings();

    // TODO: move these three checks to initialization.
    //-------------------------------------------------------------------------
    if (settings.protocol_minimum < version::level::minimum)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, minimum below ("
            << version::level::minimum << ").";
        set_event(error::channel_stopped);
        return false;
    }

    if (settings.protocol_maximum > version::level::maximum)
    {
        LOG_ERROR(LOG_NETWORK)
            << "Invalid protocol version configuration, maximum above ("
            << version::level::maximum << ").";
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

    if ((message->services() & minimum_services_) != minimum_services_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer network services (" << message->services()
            << ") for [" << authority() << "]";

        static const message::reject rejection
        {
            reject::reason_code::obsolete,
            version::command,
            reason_insufficient_services,
            null_hash
        };

        SEND2(rejection, handle_send, _1, rejection.command);
        set_event(error::channel_stopped);
        return false;
    }

    if (message->value() < minimum_version_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Insufficient peer protocol version (" << message->value()
            << ") for [" << authority() << "]";

        static const message::reject rejection
        {
            reject::reason_code::obsolete,
            version::command,
            reason_insufficient_version,
            null_hash
        };

        SEND2(rejection, handle_send, _1, rejection.command);
        set_event(error::channel_stopped);
        return false;
    }

    const auto version = std::min(message->value(), own_version_);
    set_negotiated_version(version);

    LOG_DEBUG(LOG_NETWORK)
        << "Negotiated protocol version (" << version
        << ") for [" << authority() << "]";

    SEND1(verack(), handle_verack_sent, _1);

    // 1 of 2
    set_event(error::success);
    return false;
}

bool protocol_version_31402::handle_receive_verack(const code& ec,
    verack_const_ptr)
{
    if (stopped())
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

void protocol_version_31402::handle_version_sent(const code& ec)
{
    if (stopped())
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure sending version to [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return;
    }
}

void protocol_version_31402::handle_verack_sent(const code& ec)
{
    if (stopped())
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure sending verack to [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return;
    }
}

} // namespace network
} // namespace libbitcoin
