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

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>

namespace libbitcoin {
namespace network {

class session;

class BCT_API protocol_version_31402
  : public protocol, track<protocol_version_31402>
{
public:
    typedef std::shared_ptr<protocol_version_31402> ptr;

    /// Construct a version protocol instance using configured values.
    protocol_version_31402(const session& session,
        const channel::ptr& channel);

    /// Construct a version protocol instance using parameterized services.
    protocol_version_31402(const session& session, const channel::ptr& channel,
        uint64_t own_services, uint64_t minimum_services);

    /// Perform the handshake (strand required), handler invoked on completion.
    virtual void start(result_handler&& handler);

    /// The channel is stopping (called on strand by stop subscription).
    void stopping(const code& ec) override;

protected:
    const std::string& name() const override;

    virtual messages::version version_factory() const;
    virtual bool sufficient_peer(const messages::version::ptr& message);

    virtual void complete(const code& ec);
    virtual void handle_timer(const code& ec);

    virtual void handle_send_version(const code& ec);
    virtual void handle_receive_version(const code& ec,
        const messages::version::ptr& message);

    virtual void handle_send_acknowledge(const code& ec);
    virtual void handle_receive_acknowledge(const code& ec,
        const messages::version_acknowledge::ptr& message);

    // These are thread safe (const).
    const uint32_t minimum_version_;
    const uint32_t maximum_version_;
    const uint64_t minimum_services_;
    const uint64_t maximum_services_;
    const uint64_t invalid_services_;

    // These are protected by strand.
    bool sent_version_;
    bool received_version_;
    bool received_acknowledge_;
    std::shared_ptr<result_handler> handler_;
    deadline::ptr timer_;
};

} // namespace network
} // namespace libbitcoin

#endif

