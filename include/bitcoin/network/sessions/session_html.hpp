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
#ifndef LIBBITCOIN_NETWORK_SESSION_HTML_HPP
#define LIBBITCOIN_NETWORK_SESSION_HTML_HPP

#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session_tcp.hpp>

namespace libbitcoin {
namespace network {

class net;

/// Inbound client connections session, thread safe.
template <typename Protocol>
class session_html
  : public session_tcp, protected tracker<session_html<Protocol>>
{
public:
    typedef std::shared_ptr<session_html<Protocol>> ptr;
    using options_t = typename Protocol::options_t;
    using channel_t = typename Protocol::channel_t;

    /// Construct an instance (network should be started).
    session_html(net& network, uint64_t identifier, const options_t& options,
        const std::string& name) NOEXCEPT
      : session_tcp(network, identifier, options, name),
        tracker<session_html<Protocol>>(network)
    {
    }

protected:
    /// Create a channel from the started socket.
    inline channel::ptr create_channel(
        const socket::ptr& socket) NOEXCEPT override
    {
        BC_ASSERT_MSG(stranded(), "strand");
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

        // Channel id must be created using create_key().
        return std::make_shared<channel_t>(log, socket,
            settings(), create_key(), options_);

        BC_POP_WARNING()
    }

    /// Overridden to set channel protocols (base calls from channel strand).
    inline void attach_protocols(const channel::ptr& channel) NOEXCEPT override
    {
        BC_ASSERT_MSG(channel->stranded(), "channel strand");
        BC_ASSERT_MSG(channel->paused(), "channel not paused for attach");

        const auto self = shared_from_this();
        channel->attach<Protocol>(self, options_)->start();
    }

private:
    // This is thread safe.
    options_t options_;
};

} // namespace network
} // namespace libbitcoin

#endif
