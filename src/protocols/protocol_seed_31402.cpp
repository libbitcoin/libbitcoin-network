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
#include <bitcoin/network/protocols/protocol_seed_31402.hpp>

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

#define CLASS protocol_seed_31402
static const std::string protocol_name = "seed";

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// Require three callbacks (or any error) before calling complete.
protocol_seed_31402::protocol_seed_31402(channel::ptr channel, p2p& network)
  : protocol_timer(channel, network.network_settings().channel_germination(),
      false),
    network_(network)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_seed_31402::start(result_handler handler)
{
    const auto& settings = network_.network_settings();
    const result_handler complete = BIND2(handle_seeding_complete, _1, handler);

    if (settings.host_pool_capacity == 0)
    {
        complete(error::address_not_found);
        return;
    }

    // TODO: just use a state member variable, this is stranded.

    ////const auto join_handler = synchronize(complete, 3, NAME,
    ////    synchronizer_terminate::on_error);

    ////protocol_timer::start(settings.channel_germination(), join_handler);

    ////SUBSCRIBE2(address, {}, handle_receive_address, _1, _2);
    ////send_own_address(settings);
    ////SEND1(get_address{}, handle_send_get_address, _1);
}

// Protocol.
// ----------------------------------------------------------------------------

void protocol_seed_31402::send_own_address(const settings& settings)
{
    if (settings.self.port() == 0)
    {
        set_event(error::success);
        return;
    }

    SEND1(address{ { settings.self.to_address_item() } },
        handle_send_address, _1);
}

void protocol_seed_31402::handle_seeding_complete(const code& ec,
    result_handler handler)
{
    handler(ec);
    stop(ec);
}

void protocol_seed_31402::handle_receive_address(const code& ec,
    address::ptr message)
{
    if (stopped(ec))
        return;

    LOG_DEBUG(LOG_NETWORK)
        << "Storing addresses from seed [" << authority() << "] ("
        << message->addresses.size() << ")";

    // TODO: use handler to avoid count race.
    // TODO: manage timestamps (active channels are connected < 3 hours ago).
    network_.load(message->addresses, {});
}

void protocol_seed_31402::handle_send_address(const code& ec)
{
    if (stopped(ec))
        return;

    // 1 of 3
    set_event(error::success);
}

void protocol_seed_31402::handle_send_get_address(const code& ec)
{
    if (stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure sending get_address to seed [" << authority() << "] "
            << ec.message();
        set_event(ec);
        return;
    }

    // 2 of 3
    set_event(error::success);
}

////void protocol_seed_31402::handle_store_addresses(const code& ec)
////{
////    if (stopped(ec))
////        return;
////
////    if (ec)
////    {
////        LOG_ERROR(LOG_NETWORK)
////            << "Failure storing addresses from seed [" << authority() << "] "
////            << ec.message();
////        set_event(ec);
////        return;
////    }
////
////    LOG_DEBUG(LOG_NETWORK)
////        << "Stopping completed seed [" << authority() << "] ";
////
////    // 3 of 3
////    set_event(error::channel_stopped);
////}

const std::string& protocol_seed_31402::name() const
{
    return protocol_name;
}

} // namespace network
} // namespace libbitcoin
