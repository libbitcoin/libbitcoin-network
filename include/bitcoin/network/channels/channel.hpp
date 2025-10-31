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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_HPP

#include <memory>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Abstract base channel with timers and identity.
/// See proxy base class for its thread safety constraints.
/// A channel is a proxy with timers and connection state.
class BCT_API channel
  : public proxy
{
public:
    typedef std::shared_ptr<channel> ptr;

    DELETE_COPY_MOVE(channel);

    /// Attach protocol to channel, caller must start (requires strand).
    template <class Protocol, class SessionPtr, typename... Args>
    inline typename Protocol::ptr attach(const SessionPtr& session,
        Args&&... args) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        if (!stranded())
            return nullptr;

        // Protocols are attached after channel start (read paused).
        auto protocol = std::make_shared<Protocol>(session,
            shared_from_base<channel>(), std::forward<Args>(args)...);

        // Protocol lifetime is ensured by the channel (proxy) stop subscriber.
        subscribe_stop([=](const code& ec) NOEXCEPT
        {
            protocol->stopping(ec);
        });

        return protocol;
    }

    /// Asserts/logs stopped.
    virtual ~channel() NOEXCEPT;

    /// Pause reading from the socket, stops timers (requires strand).
    void pause() NOEXCEPT override;

    /// Resume reading from the socket, starts timers (requires strand).
    void resume() NOEXCEPT override;

    /// Seconds before channel expires, zero if expired (requires strand).
    size_t remaining() const NOEXCEPT;

    /// Arbitrary nonce of the channel (for loopback guard).
    uint64_t nonce() const NOEXCEPT;

    /// Arbitrary identifier of the channel (for session subscribers).
    uint64_t identifier() const NOEXCEPT;

    /// Configuration settings.
    const network::settings& settings() const NOEXCEPT;

protected:
    /// Construct a channel to encapsulated and communicate on the socket.
    channel(const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero,
        const deadline::duration& inactivity={},
        const deadline::duration& expiration={}) NOEXCEPT;

    /// Stranded handler invoked from stop().
    void stopping(const code& ec) NOEXCEPT override;

    /// Stranded notifier, allows timer reset.
    void waiting() NOEXCEPT override;

private:
    void stop_expiration() NOEXCEPT;
    void start_expiration() NOEXCEPT;
    void handle_expiration(const code& ec) NOEXCEPT;

    void stop_inactivity() NOEXCEPT;
    void start_inactivity() NOEXCEPT;
    void handle_inactivity(const code& ec) NOEXCEPT;

    // These are thread safe (const).
    const network::settings& settings_;
    const uint64_t identifier_;
    const uint64_t nonce_
    {
        system::pseudo_random::next<uint64_t>(one, max_uint64)
    };

    // These are protected by strand.
    deadline::ptr inactivity_;
    deadline::ptr expiration_;
};

typedef std::function<void(const code&, const channel::ptr&)> channel_handler;

} // namespace network
} // namespace libbitcoin

#endif
