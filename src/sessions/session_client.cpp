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
#include <bitcoin/network/sessions/session_client.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net.hpp>
#include <bitcoin/network/sessions/session.hpp>

namespace libbitcoin {
namespace network {

session_client::session_client(net& network, uint64_t identifier) NOEXCEPT
  : network_(network), session(network, identifier)
{
}

// Channel sequence.
// ----------------------------------------------------------------------------

void session_client::attach_handshake(const channel::ptr& BC_DEBUG_ONLY(channel),
    result_handler&&) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for handshake attach");

    ////const auto self = shared_from_this();
    ////const auto client = std::dynamic_pointer_cast<channel_client>(channel);
}

// Override in derived sessions to attach protocols.
void session_client::attach_protocols(
    const channel::ptr& BC_DEBUG_ONLY(channel)) NOEXCEPT
{
    BC_ASSERT_MSG(channel->stranded(), "channel strand");
    BC_ASSERT_MSG(channel->paused(), "channel not paused for protocol attach");

    ////const auto self = shared_from_this();
    ////const auto client = std::dynamic_pointer_cast<channel_client>(channel);
}

// Factories.
// ----------------------------------------------------------------------------

acceptor::ptr session_client::create_acceptor() NOEXCEPT
{
    return network_.create_acceptor();
}

connector::ptr session_client::create_connector() NOEXCEPT
{
    return network_.create_connector();
}

connectors_ptr session_client::create_connectors(size_t count) NOEXCEPT
{
    return network_.create_connectors(count);
}

channel::ptr session_client::create_channel(const socket::ptr& socket) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Channel id must be created using create_key().
    return std::make_shared<channel_client>(log, socket, settings(), create_key());
}

} // namespace network
} // namespace libbitcoin
