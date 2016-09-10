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
#include <bitcoin/network/protocols/protocol_version_70002.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_version_31402.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "protocol_version_70002"
#define CLASS protocol_version_70002

using namespace bc::message;
using namespace std::placeholders;

message::version protocol_version_70002::version_factory(
    const config::authority& authority, const settings& settings,
    uint64_t nonce, size_t height)
{
    auto version = protocol_version_31402::version_factory(authority, settings,
        nonce, height);

    // This is the only difference at protocol level 70001.
    version.relay = settings.relay_transactions;

    return version;
}

protocol_version_70002::protocol_version_70002(p2p& network,
    channel::ptr channel)
  : protocol_version_70002(network, channel,
        network.network_settings().protocol_minimum,
        network.network_settings().services)
{
}

protocol_version_70002::protocol_version_70002(p2p& network,
    channel::ptr channel, uint32_t minimum_version, uint64_t minimum_services)
  : protocol_version_31402(network, channel, minimum_version,
        minimum_services),
    CONSTRUCT_TRACK(protocol_version_70002)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_version_70002::start(event_handler handler)
{
    const auto height = network_.height();
    const auto& settings = network_.network_settings();

    // The handler is invoked in the context of the last message receipt.
    protocol_timer::start(settings.channel_handshake(),
        synchronize(handler, 2, NAME, false));

    SUBSCRIBE2(version, handle_receive_version, _1, _2);
    SUBSCRIBE2(verack, handle_receive_verack, _1, _2);
    send_version(version_factory(authority(), settings, nonce(), height));
}

} // namespace network
} // namespace libbitcoin
