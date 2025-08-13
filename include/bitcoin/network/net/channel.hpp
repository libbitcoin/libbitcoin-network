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
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/distributor.hpp>
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

    /// Subscribe to messages from peer (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Message, typename Handler = distributor::handler<Message>>
    void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        distributor_.subscribe(std::forward<Handler>(handler));
    }

    /// Serialize and write a message to the peer (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <class Message>
    void send(const Message& message, result_handler&& complete) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        // TODO: build witness into feature w/magic and negotiated version.
        // TODO: if self and peer services show witness, set feature true.
        const auto data = messages::serialize(message, settings_.identifier,
            negotiated_version());

        if (!data)
        {
            // This is an internal error, should never happen.
            LOGF("Serialization failure (" << Message::command << ").");
            complete(error::unknown);
            return;
        }

        write(data, std::move(complete));
    }

    /// Construct a channel to encapsulated and communicate on the socket.
    channel(memory& memory, const logger& log, const socket::ptr& socket,
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

    /// Message level is supported by configured protocol level.
    bool is_negotiated(messages::level level) const NOEXCEPT;

    /// Service level is advertised by peer.
    bool is_peer_service(messages::service service) const NOEXCEPT;

    /// Start height for version message (set only before handshake).
    size_t start_height() const NOEXCEPT;
    void set_start_height(size_t height) NOEXCEPT;

    /// Negotiated version should be written only in handshake (safety).
    uint32_t negotiated_version() const NOEXCEPT;
    void set_negotiated_version(uint32_t value) NOEXCEPT;

    /// Peer version should be written only in handshake.
    messages::version::cptr peer_version() const NOEXCEPT;
    void set_peer_version(const messages::version::cptr& value) NOEXCEPT;

    /// Originating address of connection with current time and peer services.
    address_item_cptr get_updated_address() const NOEXCEPT;

protected:
    typedef messages::heading::cptr heading_ptr;

    /// Protocol-specific read and dispatch.
    void read_heading() NOEXCEPT;
    void handle_read_heading(const code& ec, size_t) NOEXCEPT;
    void handle_read_payload(const code& ec, size_t payload_size,
        const heading_ptr& head) NOEXCEPT;

    /// Notify subscribers of a new message (requires strand).
    virtual code notify(messages::identifier id, uint32_t version,
        const system::data_chunk& source) NOEXCEPT;

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

    // These are protected by strand.
    distributor distributor_;
    deadline::ptr expiration_;
    deadline::ptr inactivity_;
    uint32_t negotiated_version_;
    messages::version::cptr peer_version_{};
    size_t start_height_{};
    system::data_chunk payload_buffer_{};
    system::data_array<messages::heading::size()> heading_buffer_{};
    system::read::bytes::copy heading_reader_{ heading_buffer_ };
};

typedef std::function<void(const code&, const channel::ptr&)> channel_handler;

} // namespace network
} // namespace libbitcoin

#endif
