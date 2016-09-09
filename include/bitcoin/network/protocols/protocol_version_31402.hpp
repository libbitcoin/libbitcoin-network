/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_VERSION_31402_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_VERSION_31402_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/protocols/protocol_timer.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p;

class BCT_API protocol_version_31402
  : public protocol_timer, track<protocol_version_31402>
{
public:
    typedef std::shared_ptr<protocol_version_31402> ptr;

    /**
     * Construct a version protocol instance.
     * @param[in]  network   The network interface.
     * @param[in]  channel   The channel on which to start the protocol.
     */
    protocol_version_31402(p2p& network, channel::ptr channel);
    
    /**
     * Start the protocol.
     * @param[in]  handler  Invoked upon stop or receipt of version and verack.
     */
    virtual void start(event_handler handler);

protected:
    static message::version version_factory(
        const config::authority& authority, const settings& settings,
            uint64_t nonce, size_t height);

    virtual void send_version(const message::version& self);

    virtual void handle_version_sent(const code& ec);
    virtual void handle_verack_sent(const code& ec);

    virtual bool handle_receive_version(const code& ec,
        message::version::ptr version);
    virtual bool handle_receive_verack(const code& ec, message::verack::ptr);

    p2p& network_;
};

} // namespace network
} // namespace libbitcoin

#endif

