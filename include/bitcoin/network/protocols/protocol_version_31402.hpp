/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_version_31402
  : public protocol, protected tracker<protocol_version_31402>
{
public:
    typedef std::shared_ptr<protocol_version_31402> ptr;

    /// Construct a version protocol instance using configured values.
    protocol_version_31402(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT;

    /// Construct a version protocol instance using parameterized services.
    protocol_version_31402(const session::ptr& session,
        const channel::ptr& channel, uint64_t minimum_services,
        uint64_t maximum_services) NOEXCEPT;

    /// Perform the handshake (strand required), handler invoked on completion.
    virtual void shake(result_handler&& handler) NOEXCEPT;

    /// The channel is stopping (called on strand by stop subscription).
    void stopping(const code& ec) NOEXCEPT override;

protected:
    virtual messages::version version_factory(bool relay=false) const NOEXCEPT;
    virtual void rejection(const code& ec) NOEXCEPT;

    virtual bool complete() const NOEXCEPT;
    virtual void callback(const code& ec) NOEXCEPT;
    virtual void handle_timer(const code& ec) NOEXCEPT;

    virtual void handle_send_version(const code& ec) NOEXCEPT;
    virtual bool handle_receive_version(const code& ec,
        const messages::version::cptr& message) NOEXCEPT;

    virtual void handle_send_acknowledge(const code& ec) NOEXCEPT;
    virtual bool handle_receive_acknowledge(const code& ec,
        const messages::version_acknowledge::cptr& message) NOEXCEPT;

    // These are thread safe (const).
    const bool inbound_;
    const uint32_t minimum_version_;
    const uint32_t maximum_version_;
    const uint64_t minimum_services_;
    const uint64_t maximum_services_;
    const uint64_t invalid_services_;

private:
    // These are protected by strand.
    bool sent_version_{};
    bool received_version_{};
    bool received_acknowledge_{};
    std::shared_ptr<result_handler> handler_{};
    deadline::ptr timer_;
};

} // namespace network
} // namespace libbitcoin

#endif

