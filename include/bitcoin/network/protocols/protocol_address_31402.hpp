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
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>

namespace libbitcoin {
namespace network {

class session;

class BCT_API protocol_address_31402
  : public protocol, protected tracker<protocol_address_31402>
{
public:
    typedef std::shared_ptr<protocol_address_31402> ptr;

    protocol_address_31402(const session& session,
        const channel::ptr& channel) NOEXCEPT;

    /// Start protocol (strand required).
    void start() NOEXCEPT override;

protected:
    const std::string& name() const NOEXCEPT override;

    virtual void handle_receive_address(const code& ec,
        const messages::address::cptr& message) NOEXCEPT;
    virtual void handle_save_address(const code& ec,
        size_t accepted, size_t filtered, size_t start) NOEXCEPT;

    virtual void handle_receive_get_address(const code& ec,
        const messages::get_address::cptr& message) NOEXCEPT;
    virtual void handle_fetch_address(const code& ec,
        const messages::address::cptr& message) NOEXCEPT;

private:
    messages::address_item self() const NOEXCEPT;

    // These are thread safe (const).
    const config::authorities& blacklist_;
    const bool inbound_;
    const bool request_;

    // These are protected by strand.
    bool received_;
    bool sent_;
};

} // namespace network
} // namespace libbitcoin

#endif
