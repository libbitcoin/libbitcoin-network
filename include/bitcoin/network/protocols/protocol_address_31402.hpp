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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_ADDRESS_31402_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_ADDRESS_31402_HPP

#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_events.hpp>

namespace libbitcoin {
namespace network {

class p2p;

/// Address protocol.
/// Attach this to a channel immediately following handshake completion.
class BCT_API protocol_address_31402
  : public protocol_events, track<protocol_address_31402>
{
public:
    typedef std::shared_ptr<protocol_address_31402> ptr;

    protocol_address_31402(channel::ptr channel, p2p& network);

    virtual void start();

protected:
    virtual void handle_stop(const code& ec);
    virtual void handle_store_addresses(const code& ec);

    virtual bool handle_receive_address(const code& ec,
        system::address_const_ptr address);
    virtual bool handle_receive_get_address(const code& ec,
        system::get_address_const_ptr message);

    virtual const std::string& name() const override;

    p2p& network_;
    const system::messages::address self_;
};

} // namespace network
} // namespace libbitcoin

#endif
