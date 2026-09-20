/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_PEER_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_PEER_HPP

#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/interfaces/interfaces.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/messages/peer/registry.hpp>

namespace libbitcoin {
namespace network {

/// Full duplex bitcoin peer-to-peer tcp/ip channel.
/// Version into should only be written before/during handshake.
/// Attach/resume/signal_activity must be called from the strand.
class BCT_API channel_peer
  : public channel
{
public:
    typedef std::shared_ptr<channel_peer> ptr;
    using counters = messages::peer::registry::counters_t;
    using options_t = settings_t::tcp_server;
    using interface = rpc::interface::peer::dispatch;
    using dispatcher = rpc::dispatcher<interface>;

    /// Subscribe to messages from peer (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Message>
    inline void subscribe(auto&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());
        using signature = interface::signature<Message>;
        dispatcher_.subscribe(std::forward<signature>(handler));
    }

    /// Write message to peer (requires strand).
    /// The message is translated to the wire by the body (transport framed).
    /// Completion handler is always invoked on the channel strand.
    template <class Message>
    inline void send(const Message& message, result_handler&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());
        using namespace messages::peer;
        using namespace std::placeholders;

        constexpr auto index = messages::peer::registry::index_of<Message>();

        frame out{};
        out.magic = settings().identifier;
        out.version = negotiated_version();
        out.message = rpc::any_t{ system::to_shared(message) };
        out.index = index;
        out.size = message.size(out.version);

        LOGX("Send " << Message::command << " to [" << endpoint() << "] ("
            << out.size << " bytes)");

        write(std::move(out),
            std::bind(&channel_peer::handle_send,
                shared_from_base<channel_peer>(), _1, _2, index,
                std::move(handler)));
    }

    /// Construct a p2p channel to encapsulate and communicate on the socket.
    inline channel_peer(const logger& log, const socket::ptr& socket,
        uint64_t identifier, const settings_t& settings,
        const options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options),
        negotiated_version_(settings.protocol_maximum)
    {
    }

    /// Resume reading from the socket, starts timers (requires strand).
    void resume() NOEXCEPT override;

    /// Quiet should be written only in handshake.
    /// The channel does not "speak" to peers (e.g. seed connection).
    bool quiet() const NOEXCEPT;
    void set_quiet() NOEXCEPT;

    /// Message level is supported by configured protocol level.
    bool is_negotiated(messages::peer::level level) const NOEXCEPT;

    /// Service level is advertised by peer.
    bool is_peer_service(messages::peer::service service) const NOEXCEPT;

    /// Start height for version message (set only before handshake).
    size_t start_height() const NOEXCEPT;
    void set_start_height(size_t height) NOEXCEPT;

    /// Negotiated version should be written only in handshake (safety).
    uint32_t negotiated_version() const NOEXCEPT;
    void set_negotiated_version(uint32_t value) NOEXCEPT;

    /// Peer accepts address v2, written only in handshake (bip155).
    bool wants_address_v2() const NOEXCEPT;
    void set_wants_address_v2() NOEXCEPT;

    /// Chain is current, reduces the read buffer to the configured minimum.
    bool current() const NOEXCEPT;
    void set_current(bool value) NOEXCEPT;

    /// Bytes sent to the peer of each registered message.
    const counters& sent_by_message() const NOEXCEPT;

    /// Bytes received from the peer of each registered message.
    const counters& received_by_message() const NOEXCEPT;

    /// Round trip time of the last ping, zero if none completed.
    steady_clock::duration ping_time() const NOEXCEPT;

    /// Least round trip time of any ping, zero if none completed.
    steady_clock::duration minimum_ping_time() const NOEXCEPT;

    /// Elapsed time of the outstanding ping, zero if none outstanding.
    steady_clock::duration pending_ping_time() const NOEXCEPT;

    /// Stamp the outstanding ping, and time it out upon its pong.
    void set_ping() NOEXCEPT;
    void set_pong() NOEXCEPT;

    /// Least fee rate of a transaction announced to the peer (bip133).
    uint64_t minimum_fee() const NOEXCEPT;
    void set_minimum_fee(uint64_t value) NOEXCEPT;

    /// Peer version should be written only in handshake.
    messages::peer::version::cptr peer_version() const NOEXCEPT;
    void set_peer_version(const messages::peer::version::cptr& value) NOEXCEPT;

    /// Originating address of connection with current time and peer services.
    address_item_cptr get_updated_address() const NOEXCEPT;

protected:
    /// Stranded handler invoked from channel::stop().
    void stopping(const code& ec) NOEXCEPT override;

    /// Construct a frame stamped with parse context.
    virtual messages::peer::frame_ptr create_frame() const NOEXCEPT;

    /// Message read and dispatch (framing is owned by peer::body).
    void receive() NOEXCEPT;
    void handle_receive(const code& ec, size_t bytes,
        const messages::peer::frame_ptr& in) NOEXCEPT;

    /// For protocol version context.
    bool is_handshaked() const NOEXCEPT;

private:
    static void count(counters& counts, size_t index, size_t bytes) NOEXCEPT;

    void log_fault(const code& ec,
        const messages::peer::frame& in) const NOEXCEPT;
    void handle_send(const code& ec, size_t size, size_t index,
        const result_handler& handler) NOEXCEPT;

    // These are protected by strand/order.
    uint32_t negotiated_version_;
    messages::peer::version::cptr peer_version_{};
    system::data_chunk payload_buffer_{};
    dispatcher dispatcher_{};
    size_t start_height_{};
    counters sent_by_message_{};
    counters received_by_message_{};
    steady_clock::time_point pinged_{};
    steady_clock::duration ping_{};
    steady_clock::duration minimum_ping_{};
    bool reading_{};
    bool quiet_{};
    bool wants_address_v2_{};
    bool current_{};

    // This is thread safe.
    std::atomic<uint64_t> minimum_fee_{};
};

} // namespace network
} // namespace libbitcoin

#endif
