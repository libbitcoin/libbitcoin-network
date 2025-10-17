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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_TCP_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_TCP_HPP

#include <memory>
#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_tcp
  : public protocol
{
public:
    typedef std::shared_ptr<protocol_tcp> ptr;
    using options_t = settings::tcp_server;
    using channel_t = channel_tcp;

protected:
    protocol_tcp(const session::ptr& session, const channel::ptr& channel,
        const options_t&) NOEXCEPT
      : protocol(session, channel)
    {
    }
};

} // namespace network
} // namespace libbitcoin

#endif
