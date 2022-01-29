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
#include <bitcoin/network/protocols/protocol.hpp>

#include <cstdint>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace messages;

protocol::protocol(channel::ptr channel)
  : channel_(channel)
{
}

protocol::~protocol()
{
}

// This nop holds protocol shared pointer in attach closure.
void protocol::nop() volatile
{
}

bool protocol::stranded() const
{
    return channel_->stranded();
}

// Protocol start methods conventionally do not invoke the passed handler.

config::authority protocol::authority() const
{
    return channel_->authority();
}

uint64_t protocol::nonce() const
{
    return channel_->nonce();
}

version::ptr protocol::peer_version() const
{
    return channel_->peer_version();
}

void protocol::set_peer_version(version::ptr value)
{
    channel_->set_peer_version(value);
}

uint32_t protocol::negotiated_version() const
{
    return channel_->negotiated_version();
}

void protocol::set_negotiated_version(uint32_t value)
{
    channel_->set_negotiated_version(value);
}

// Stop the channel.
void protocol::stop(const code& ec)
{
    channel_->stop(ec);
}

// protected
void protocol::handle_send(const code&, const std::string&)
{
    // Send and receive failures are logged by the proxy.
    // This provides a convenient default overridable handler.
    BC_ASSERT_MSG(stranded(), "strand");
}

} // namespace network
} // namespace libbitcoin
