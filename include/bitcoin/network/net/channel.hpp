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
#ifndef LIBBITCOIN_NETWORK_NET_CHANNEL_HPP
#define LIBBITCOIN_NETWORK_NET_CHANNEL_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/broadcaster.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class session;

/// Virtual, thread safe except for:
/// * See proxy for its thread safety constraints.
/// * Version into should only be written before/during handshake.
/// * attach/resume/signal_activity must be called from the strand.
/// A channel is a proxy with timers and connection state.
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
        const settings& settings, uint64_t identifier=zero,
        bool quiet=true) NOEXCEPT;

    /// Asserts/logs stopped.
    virtual ~channel() NOEXCEPT;

    /// Idempotent, may be called multiple times.
    void stop(const code& ec) NOEXCEPT override;

    /// Pause reading from the socket, stops timers (requires strand).
    void pause() NOEXCEPT override;

    /// Resume reading from the socket, starts timers (requires strand).
    void resume() NOEXCEPT override;

    /// The channel does not "speak" to peers (e.g. seed connection).
    bool quiet() const NOEXCEPT;

    /// Arbitrary nonce of the channel (for loopback guard).
    uint64_t nonce() const NOEXCEPT;

    /// Arbitrary identifier of the channel (for session subscribers).
    uint64_t identifier() const NOEXCEPT;

    /// Start height for version message (set only before handshake).
    size_t start_height() const NOEXCEPT;
    void set_start_height(size_t height) NOEXCEPT;

    /// Negotiated version should be written only in handshake.
    uint32_t negotiated_version() const NOEXCEPT;
    void set_negotiated_version(uint32_t value) NOEXCEPT;

    /// Peer version should be written only in handshake.
    messages::version::cptr peer_version() const NOEXCEPT;
    void set_peer_version(const messages::version::cptr& value) NOEXCEPT;

    /// Originating address of connection with current time and peer services.
    address_item_cptr get_updated_address() const NOEXCEPT;

protected:
    /// Property values provided to the proxy.
    size_t minimum_buffer() const NOEXCEPT override;
    size_t maximum_payload() const NOEXCEPT override;
    uint32_t protocol_magic() const NOEXCEPT override;
    bool validate_checksum() const NOEXCEPT override;
    uint32_t version() const NOEXCEPT override;

    /// Signals inbound traffic, called from proxy on strand (requires strand).
    void signal_activity() NOEXCEPT override;

private:
    bool is_handshaked() const NOEXCEPT;
    void do_stop(const code& ec) NOEXCEPT;

    void stop_expiration() NOEXCEPT;
    void start_expiration() NOEXCEPT;
    void handle_expiration(const code& ec) NOEXCEPT;

    void stop_inactivity() NOEXCEPT;
    void start_inactivity() NOEXCEPT;
    void handle_inactivity(const code& ec) NOEXCEPT;

    // Proxy base class is not fully thread safe.

    // These are thread safe (const).
    const bool quiet_;
    const settings& settings_;
    const uint64_t identifier_;
    const uint64_t nonce_
    {
        system::pseudo_random::next<uint64_t>(one, max_uint64)
    };

    // These are not thread safe.
    deadline::ptr expiration_;
    deadline::ptr inactivity_;
    uint32_t negotiated_version_;
    messages::version::cptr peer_version_{};
    size_t start_height_{};
};

typedef std::function<void(const code&, const channel::ptr&)> channel_handler;

} // namespace network
} // namespace libbitcoin

#endif
