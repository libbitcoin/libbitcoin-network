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
#ifndef LIBBITCOIN_NETWORK_NET_CHANNEL_HPP
#define LIBBITCOIN_NETWORK_NET_CHANNEL_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class session;

/// Virtual, thread safe except for:
/// * attach/resume/signal_activity must be called from the channel strand.
/// * Versions should be only written in handshake and read thereafter.
/// * See proxy for its thread safety constraints.
/// A channel is a proxy with logged timers and state.
class BCT_API channel
  : public proxy, protected track<channel>
{
public:
    DELETE_COPY_MOVE(channel);

    typedef std::shared_ptr<channel> ptr;

    /// Attach protocol to channel, caller must start (requires strand).
    template <class Protocol, class Session, typename... Args>
    typename Protocol::ptr attach(const Session& session,
        Args&&... args) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "subscribe_stop");

        if (!stranded())
            return nullptr;

        // Protocols are attached after channel start (read paused).
        const auto protocol = std::make_shared<Protocol>(session,
            shared_from_base<channel>(), std::forward<Args>(args)...);

        // Protocol lifetime is ensured by the channel stop subscriber.
        subscribe_stop([=](const code& ec) NOEXCEPT
        {
            protocol->stopping(ec);
        });

        return protocol;
    }

    channel(const logger& log, const socket::ptr& socket,
        const settings& settings) NOEXCEPT;
    virtual ~channel() NOEXCEPT;

    /// Arbitrary nonce of the channel (for loopback guard).
    uint64_t nonce() const NOEXCEPT;

    /// Versions should be only written in handshake and read thereafter.
    void set_negotiated_version(uint32_t value) NOEXCEPT;
    uint32_t negotiated_version() const NOEXCEPT;
    void set_peer_version(const messages::version::ptr& value) NOEXCEPT;
    messages::version::ptr peer_version() const NOEXCEPT;

    /// Resume reading from the socket (requires strand).
    void resume() NOEXCEPT override;

    /// Idempotent, may be called multiple times.
    void stop(const code& ec) NOEXCEPT override;

protected:
    /// Property values provided to the proxy.
    size_t maximum_payload() const NOEXCEPT override;
    uint32_t protocol_magic() const NOEXCEPT override;
    bool validate_checksum() const NOEXCEPT override;
    bool verbose() const NOEXCEPT override;
    uint32_t version() const NOEXCEPT override;

    /// Signals inbound traffic, called from proxy on strand (requires strand).
    void signal_activity() NOEXCEPT override;

private:
    void do_stop(const code& ec) NOEXCEPT;

    void start_expiration() NOEXCEPT;
    void handle_expiration(const code& ec) NOEXCEPT;

    void start_inactivity() NOEXCEPT;
    void handle_inactivity(const code& ec) NOEXCEPT;

    // Proxy base class is not fully thread safe.

    // These are thread safe.
    const size_t maximum_payload_;
    const uint32_t protocol_magic_;
    const uint64_t channel_nonce_;
    const bool validate_checksum_;
    const bool verbose_logging_;

    // These are not thread safe.
    uint32_t negotiated_version_;
    messages::version::ptr peer_version_;
    deadline::ptr expiration_;
    deadline::ptr inactivity_;
};

} // namespace network
} // namespace libbitcoin

#endif
