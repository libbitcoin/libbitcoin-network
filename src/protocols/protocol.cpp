/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/p2p.hpp>

namespace libbitcoin {
namespace network {

#define NAME "protocol"

protocol::protocol(p2p& network, channel::ptr channel, const std::string& name)
  : pool_(network.thread_pool()),
    dispatch_(network.thread_pool(), NAME),
    channel_(channel),
    name_(name)
{
}

config::authority protocol::authority() const
{
    return channel_->authority();
}

const std::string& protocol::name() const
{
    return name_;
}

uint64_t protocol::nonce() const
{
    return channel_->nonce();
}

version_const_ptr protocol::peer_version() const
{
    return channel_->peer_version();
}

void protocol::set_peer_version(version_const_ptr value)
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

threadpool& protocol::pool()
{
    return pool_;
}

// Stop the channel.
void protocol::stop(const code& ec)
{
    channel_->stop(ec);
}

// protected
void protocol::handle_send(const code& , const std::string& )
{
    // Send and receive failures are logged by the proxy.
    // This provides a convenient location for override if desired.
}

} // namespace network
} // namespace libbitcoin
