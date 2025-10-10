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
#include <bitcoin/network/sessions/session_client_inbound.hpp>

#include <utility>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net.hpp>
#include <bitcoin/network/protocols/protocols.hpp>
#include <bitcoin/network/sessions/session_client.hpp>

namespace libbitcoin {
namespace network {

#define LOGGING_NAME "admin"
#define CLASS session_client_inbound

using namespace system;
using namespace std::placeholders;

session_client_inbound::session_client_inbound(net& network,
    uint64_t identifier) NOEXCEPT
  : session_client(network, identifier, network.network_settings().admin.binds,
      network.network_settings().admin.connections, LOGGING_NAME),
    tracker<session_client_inbound>(network.log)
{
}

void session_client_inbound::attach_protocols(
    const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for protocol attach");

    const auto self = shared_from_this();
    channel->attach<protocol_client>(self, settings().admin)->start();
}

// TODO: channel_client_inbound
channel::ptr session_client_inbound::create_channel(
    const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

    // Channel id must be created using create_key().
    return std::make_shared<channel_client>(log, socket, settings(),
        create_key());

    BC_POP_WARNING()
}


} // namespace network
} // namespace libbitcoin
