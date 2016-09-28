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

#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_version_31402.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

protocol_version_70002::protocol_version_70002(p2p& network,
    channel::ptr channel)
  : protocol_version_70002(network, channel,
        network.network_settings().protocol_maximum,
        network.network_settings().services,
        network.network_settings().protocol_minimum,
        network.network_settings().services,
        network.network_settings().relay_transactions)
{
}

protocol_version_70002::protocol_version_70002(p2p& network,
    channel::ptr channel, uint32_t own_version, uint64_t own_services,
    uint32_t minimum_version, uint64_t minimum_services, bool relay)
  : protocol_version_31402(network, channel, own_version, own_services,
        minimum_version, minimum_services),
    relay_(relay),
    CONSTRUCT_TRACK(protocol_version_70002)
{
}

message::version protocol_version_70002::version_factory() const
{
    auto version = protocol_version_31402::version_factory();

    // This is the only difference at protocol level 70001.
    version.set_relay(relay_);
    return version;
}

} // namespace network
} // namespace libbitcoin
