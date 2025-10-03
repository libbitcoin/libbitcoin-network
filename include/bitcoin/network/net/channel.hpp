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
#ifndef LIBBITCOIN_NETWORK_NET_CHANNEL_HPP
#define LIBBITCOIN_NETWORK_NET_CHANNEL_HPP

#include <memory>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class BCT_API channel
  : public proxy, protected tracker<channel>
{
public:
    typedef std::shared_ptr<channel> ptr;

    DELETE_COPY_MOVE(channel);

    /// Attach protocol to channel, caller must start (requires strand).
    template <class Protocol, class SessionPtr, typename... Args>
    typename Protocol::ptr attach(const SessionPtr& session,
        Args&&... args) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        if (!stranded())
            return nullptr;

        // Protocols are attached after channel start (read paused).
        const auto protocol = std::make_shared<Protocol>(session,
            shared_from_base<channel>(), std::forward<Args>(args)...);

        // Protocol lifetime is ensured by the channel (proxy) stop subscriber.
        subscribe_stop([=](const code& ec) NOEXCEPT
        {
            protocol->stopping(ec);
        });

        return protocol;
    }

    /// Construct a channel to encapsulated and communicate on the socket.
    channel(const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero) NOEXCEPT;

    /// Asserts/logs stopped.
    virtual ~channel() NOEXCEPT;

    /// Arbitrary nonce of the channel (for loopback guard).
    uint64_t nonce() const NOEXCEPT;

    /// Arbitrary identifier of the channel (for session subscribers).
    uint64_t identifier() const NOEXCEPT;

    /// Configuration settings.
    network::settings settings() const NOEXCEPT;

private:
    // These are thread safe (const).
    const network::settings& settings_;
    const uint64_t identifier_;
    const uint64_t nonce_
    {
        system::pseudo_random::next<uint64_t>(one, max_uint64)
    };
};

typedef std::function<void(const code&, const channel::ptr&)> channel_handler;

} // namespace network
} // namespace libbitcoin

#endif
