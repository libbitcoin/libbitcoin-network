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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_SEED_31402_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_SEED_31402_HPP

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
class BCT_API protocol_seed_31402
  : public protocol, protected tracker<protocol_seed_31402>
{
public:
    typedef std::shared_ptr<protocol_seed_31402> ptr;

    protocol_seed_31402(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT;

    /// Perform seeding, stops channel on completion (strand required).
    void start() NOEXCEPT override;

    /// Capture stop subscription to clear timer.
    void stopping(const code& ec) NOEXCEPT override;

protected:
    virtual bool complete() const NOEXCEPT;
    virtual void handle_timer(const code& ec) NOEXCEPT;

    virtual messages::address::cptr filter(
        const messages::address_items& message) const NOEXCEPT;

    virtual void handle_send_get_address(const code& ec) NOEXCEPT;
    virtual bool handle_receive_address(const code& ec,
        const messages::address::cptr& address) NOEXCEPT;
    virtual void handle_save_addresses(const code& ec,
        size_t accepted, size_t filtered, size_t start_size) NOEXCEPT;

    virtual bool handle_receive_get_address(const code& ec,
        const messages::get_address::cptr& message) NOEXCEPT;
    virtual void handle_send_address(const code& ec) NOEXCEPT;

private:
    // These are protected by the strand.
    bool sent_address_{};
    bool sent_get_address_{};
    bool received_address_{};
    deadline::ptr timer_;
};

} // namespace network
} // namespace libbitcoin

#endif
