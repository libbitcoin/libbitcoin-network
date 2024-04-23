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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_ADDRESS_IN_31402_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_ADDRESS_IN_31402_HPP

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

class BCT_API protocol_address_in_31402
  : public protocol, protected tracker<protocol_address_in_31402>
{
public:
    typedef std::shared_ptr<protocol_address_in_31402> ptr;

    protocol_address_in_31402(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT;

    /// Start protocol (strand required).
    void start() NOEXCEPT override;

protected:
    virtual messages::address::cptr filter(
        const messages::address_items& message) const NOEXCEPT;

    virtual bool handle_receive_address(const code& ec,
        const messages::address::cptr& message) NOEXCEPT;
    virtual void handle_save_address(const code& ec,
        size_t accepted, size_t filtered, size_t start_size) NOEXCEPT;

private:
    // This is thread safe (const).
    const bool outbound_;

    // This is protected by strand.
    bool first_{ true };
};

} // namespace network
} // namespace libbitcoin

#endif
