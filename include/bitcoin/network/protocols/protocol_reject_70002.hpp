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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_REJECT_70002_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_REJECT_70002_HPP

#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/protocols/protocol_events.hpp>

namespace libbitcoin {
namespace network {

class p2p;

class BCT_API protocol_reject_70002
  : public protocol_events, track<protocol_reject_70002>
{
public:
    typedef std::shared_ptr<protocol_reject_70002> ptr;

    /**
     * Construct a reject protocol for logging reject payloads.
     * @param[in]  network   The network interface.
     * @param[in]  channel   The channel for the protocol.
     */
    protocol_reject_70002(p2p& network, channel::ptr channel);

    /**
     * Start the protocol.
     */
    virtual void start();

protected:
    virtual bool handle_receive_reject(const code& ec,
        reject_const_ptr reject);
};

} // namespace network
} // namespace libbitcoin

#endif

