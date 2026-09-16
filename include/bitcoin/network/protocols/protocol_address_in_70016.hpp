/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_ADDRESS_IN_70016_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_ADDRESS_IN_70016_HPP

#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_address_in_209.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_address_in_70016
  : public protocol_address_in_209,
    protected tracker<protocol_address_in_70016>
{
public:
    typedef std::shared_ptr<protocol_address_in_70016> ptr;

    protocol_address_in_70016(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT;

protected:
    void subscribe_address() NOEXCEPT override;

    virtual bool handle_receive_address_v2(const code& ec,
        const messages::peer::address_v2::cptr& message) NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
