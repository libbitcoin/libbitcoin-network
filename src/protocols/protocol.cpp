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
#include <boost/asio.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/net/net.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace messages;

protocol::protocol(const session& session, channel::ptr channel)
  : channel_(channel), session_(session)
{
}

// This nop holds protocol shared pointer in attach closure.
void protocol::nop() volatile noexcept
{
}

bool protocol::stranded() const
{
    return channel_->stranded();
}

bool protocol::stopped() const
{
    return channel_->stopped();
}

config::authority protocol::authority() const
{
    return channel_->authority();
}

uint64_t protocol::nonce() const noexcept
{
    return channel_->nonce();
}

version::ptr protocol::peer_version() const noexcept
{
    return channel_->peer_version();
}

void protocol::set_peer_version(version::ptr value) noexcept
{
    channel_->set_peer_version(value);
}

uint32_t protocol::negotiated_version() const noexcept
{
    return channel_->negotiated_version();
}

void protocol::set_negotiated_version(uint32_t value) noexcept
{
    channel_->set_negotiated_version(value);
}

void protocol::stop(const code& ec)
{
    channel_->stop(ec);
}

void protocol::handle_send(const code&, const std::string&)
{
    // Send and receive failures are logged by the proxy.
    // This provides a convenient default overridable handler.
    BC_ASSERT_MSG(stranded(), "strand");
}

const network::settings& protocol::settings() const
{
    return session_.settings();
}

void protocol::saves(const messages::address_items& addresses,
    result_handler handler)
{
    // boost::asio::bind_executor not working.
    boost::asio::post(channel_->strand(),
        std::bind(&protocol::do_saves,
            shared_from_this(), addresses, handler));
}

void protocol::do_saves(const messages::address_items& addresses,
    result_handler handler)
{
    session_.saves(addresses, handler);
}

void protocol::fetches(fetches_handler handler)
{
    // boost::asio::bind_executor not working.
    boost::asio::post(channel_->strand(),
        std::bind(&protocol::do_fetches,
            shared_from_this(), handler));
}

void protocol::do_fetches(fetches_handler handler)
{
    session_.fetches(handler);
}

} // namespace network
} // namespace libbitcoin
