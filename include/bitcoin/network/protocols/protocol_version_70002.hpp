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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_VERSION_70002_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_VERSION_70002_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_version_31402.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p;

class BCT_API protocol_version_70002
  : public protocol_version_31402, track<protocol_version_70002>
{
public:
    typedef std::shared_ptr<protocol_version_70002> ptr;

    /// Construct a version protocol instance using configured minimums.
    protocol_version_70002(channel::ptr channel, p2p& network);

    /// Construct a version protocol instance.
    protocol_version_70002(channel::ptr channel, p2p& network,
        uint32_t own_version, uint64_t own_services, uint64_t invalid_services,
        uint32_t minimum_version, uint64_t minimum_services, bool relay);

    void start(event_handler handle_event) override;

protected:
    system::messages::version version_factory() const override;
    bool sufficient_peer(system::version_const_ptr message) override;

    virtual bool handle_receive_reject(const code& ec,
        system::reject_const_ptr reject);

    virtual const std::string& name() const override;

    const bool relay_;
};

} // namespace network
} // namespace libbitcoin

#endif
