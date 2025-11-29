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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_PEER_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_PEER_HPP

#include <memory>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>
#include <bitcoin/network/rpc/rpc.hpp>

namespace libbitcoin {
namespace network {

/// Full duplex bitcoin peer-to-peer tcp/ip channel.
/// Version into should only be written before/during handshake.
/// Attach/resume/signal_activity must be called from the strand.
class BCT_API channel_peer
  : public channel, protected tracker<channel_peer>
{
public:
    typedef std::shared_ptr<channel_peer> ptr;
    ////using options_t = settings_t::tcp_server;
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

    /// Serialize and write message to peer (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <class Message>
    inline void send(const Message& message, result_handler&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());

        // TODO: move to serializer.
        const auto magic = settings().identifier;
        const auto version = negotiated_version();
        const auto ptr = messages::peer::serialize(message, magic, version);

        using namespace std::placeholders;
        count_handler complete = std::bind(&channel_peer::handle_send,
            shared_from_base<channel_peer>(),  _1, _2, ptr, std::move(handler));

        if (!ptr)
            complete(error::bad_alloc, {});
        else
            write({ ptr->data(), ptr->size() }, std::move(complete));
    }

    /// Construct a p2p channel to encapsulate and communicate on the socket.
    inline channel_peer(const logger& log, const socket::ptr& socket,
        uint64_t identifier, const settings_t& settings,
        const options_t& options) NOEXCEPT
      : channel_peer(mallocator_, log, socket, identifier, settings, options)
    {
    }

    /// Construct a p2p channel to encapsulate and communicate on the socket.
    inline channel_peer(memory& allocator, const logger& log,
        const socket::ptr& socket, uint64_t identifier,
        const settings_t& settings, const options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options),
        allocator_(allocator),
        negotiated_version_(settings.protocol_maximum),
        tracker<channel_peer>(log)
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

    /// Peer version should be written only in handshake.
    messages::peer::version::cptr peer_version() const NOEXCEPT;
    void set_peer_version(const messages::peer::version::cptr& value) NOEXCEPT;

    /// Originating address of connection with current time and peer services.
    address_item_cptr get_updated_address() const NOEXCEPT;

protected:
    typedef messages::peer::heading::cptr heading_ptr;

    /// Stranded handler invoked from channel::stop().
    void stopping(const code& ec) NOEXCEPT override;

    /// Protocol-specific read and dispatch.
    void read_heading() NOEXCEPT;
    void handle_read_heading(const code& ec, size_t) NOEXCEPT;
    void handle_read_payload(const code& ec, size_t payload_size,
        const heading_ptr& head) NOEXCEPT;

    /// For protocol version context.
    bool is_handshaked() const NOEXCEPT;

private:
    void log_message(const std::string_view& name, size_t size) const NOEXCEPT;
    void handle_send(const code& ec, size_t size, const system::chunk_cptr&,
        const result_handler& handler) NOEXCEPT;

    // Only passes static member get_area(), so safe to use statically.
    static default_memory mallocator_;

    // These are protected by strand/order.
    memory& allocator_;
    uint32_t negotiated_version_;
    messages::peer::version::cptr peer_version_{};
    dispatcher dispatcher_{};
    size_t start_height_{};
    bool quiet_{};

    system::data_chunk payload_buffer_{};
    system::data_array<messages::peer::heading::size()> heading_buffer_{};

    // Because heading buffer is fixed the stream can be reused as well.
    system::stream::in::fast heading_stream_{ heading_buffer_ };
    system::read::bytes::fast heading_reader_{ heading_stream_ };
};

} // namespace network
} // namespace libbitcoin

#endif
