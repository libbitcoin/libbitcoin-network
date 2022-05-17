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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_REJECT_70002_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_REJECT_70002_HPP

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

class BCT_API protocol_reject_70002
  : public protocol, track<protocol_reject_70002>
{
public:
    typedef std::shared_ptr<protocol_reject_70002> ptr;

    protocol_reject_70002(const session& session, const channel::ptr& channel);

    /// Start protocol (strand required).
    void start() override;

protected:
    const std::string& name() const override;

    virtual void handle_receive_reject(const code& ec,
        const messages::reject::ptr& reject);
};

} // namespace network
} // namespace libbitcoin

#endif

