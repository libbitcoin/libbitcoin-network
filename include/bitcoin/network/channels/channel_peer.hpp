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

#include <memory>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/interfaces/interfaces.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/messages.hpp>

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

        frame out{};
        out.magic = settings().identifier;
        out.version = negotiated_version();
        out.message = rpc::any_t{ system::to_shared(message) };
        out.index = rpc::peer_registry::index_of<Message>();

        LOGX("Send " << Message::command << " to [" << endpoint() << "] ("
            << message.size(out.version) << " bytes)");

        write(std::move(out),
            std::bind(&channel_peer::handle_send,
                shared_from_base<channel_peer>(), _1, _2, Message::command,
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

    /// Chain is current, reduces the read buffer to the configured minimum.
    bool current() const NOEXCEPT;
    void set_current(bool value) NOEXCEPT;

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
    void log_fault(const code& ec,
        const messages::peer::frame& in) const NOEXCEPT;
    void handle_send(const code& ec, size_t size,
        const std::string& command, const result_handler& handler) NOEXCEPT;

    // These are protected by strand/order.
    uint32_t negotiated_version_;
    messages::peer::version::cptr peer_version_{};
    system::data_chunk payload_buffer_{};
    dispatcher dispatcher_{};
    size_t start_height_{};
    bool reading_{};
    bool quiet_{};
    bool current_{};
};

} // namespace network
} // namespace libbitcoin

#endif
