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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_VERSION_31402_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_VERSION_31402_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
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

    /// Construct a version protocol instance using configured minimums.
    protocol_version_31402(const session& session, channel::ptr channel);

    /// Construct a version protocol instance.
    protocol_version_31402(const session& session, channel::ptr channel,
        uint32_t own_version, uint64_t own_services, uint64_t invalid_services,
        uint32_t minimum_version, uint64_t minimum_services);

    void start(result_handler handle_event) override;

protected:
    // Expose polymorphic start method from base.
    using protocol_timer::start;

    virtual messages::version version_factory() const;
    virtual bool sufficient_peer(messages::version::ptr message);

    virtual void handle_receive_version(const code& ec,
        messages::version::ptr version);
    virtual void handle_receive_acknowledge(const code& ec,
        messages::version_acknowledge::ptr);

    const std::string& name() const override;

    const uint32_t own_version_;
    const uint64_t own_services_;
    const uint64_t invalid_services_;
    const uint32_t minimum_version_;
    const uint64_t minimum_services_;
};

} // namespace network
} // namespace libbitcoin

#endif

