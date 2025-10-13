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
#ifndef LIBBITCOIN_NETWORK_SESSION_HTTP_HPP
#define LIBBITCOIN_NETWORK_SESSION_HTTP_HPP

#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session_client.hpp>

namespace libbitcoin {
namespace network {

class net;

/// Inbound client connections session, thread safe.
class BCT_API session_html
  : public session_client, protected tracker<session_html>
{
public:
    typedef std::shared_ptr<session_html> ptr;

    /// Construct an instance (network should be started).
    session_html(net& network, uint64_t identifier) NOEXCEPT;

protected:
    /// Create a channel from the started socket.
    channel::ptr create_channel(const socket::ptr& socket) NOEXCEPT override;

    /// Overridden to change channel protocols (base calls from channel strand).
    void attach_protocols(const channel::ptr& channel) NOEXCEPT override;
};

} // namespace network
} // namespace libbitcoin

#endif
