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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_CLIENT_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_CLIENT_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

class BCT_API protocol_client
  : public protocol
{
protected:
    typedef std::shared_ptr<protocol_client> ptr;

    /// Construct an instance.
    protocol_client(const session::ptr& session,
        const channel::ptr& channel) NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
