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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_VERSION_70016_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_VERSION_70016_HPP

#include <memory>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_version_70002.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_version_70016
  : public protocol_version_70002, protected tracker<protocol_version_70016>
{
public:
    typedef std::shared_ptr<protocol_version_70016> ptr;

    /// Construct a version protocol instance using configured values.
    protocol_version_70016(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT;

    /// Construct a version protocol instance using parameterized services.
    protocol_version_70016(const session::ptr& session,
        const channel::ptr& channel, uint64_t minimum_services,
        uint64_t maximum_services, bool relay, bool reject) NOEXCEPT;

    /// Perform the handshake (strand required), handler invoked on completion.
    void shake(result_handler&& handle_event) NOEXCEPT override;

protected:
    void handle_send_version(const code& ec) NOEXCEPT override;
    bool handle_receive_acknowledge(const code& ec,
        const messages::peer::version_acknowledge::cptr& message) NOEXCEPT override;
    virtual bool handle_receive_send_address_v2(const code& ec,
        const messages::peer::send_address_v2::cptr& message) NOEXCEPT;
    virtual bool handle_receive_witness_tx_id_relay(const code& ec,
        const messages::peer::witness_tx_id_relay::cptr& message) NOEXCEPT;

private:
    // This is thread safe.
    const bool reject_;

    // This is protected by strand.
    bool complete_{};
};

} // namespace network
} // namespace libbitcoin

#endif
