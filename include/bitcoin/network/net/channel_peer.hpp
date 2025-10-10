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
#ifndef LIBBITCOIN_NETWORK_NET_CHANNEL_PEER_HPP
#define LIBBITCOIN_NETWORK_NET_CHANNEL_PEER_HPP

#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/net/distributor_peer.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Virtual, thread safe except for:
/// * See proxy for its thread safety constraints.
/// * Version into should only be written before/during handshake.
/// * attach/resume/signal_activity must be called from the strand.
/// A channel is a proxy with timers and connection state.
class BCT_API channel_peer
  : public channel, protected tracker<channel_peer>
{
public:
    typedef std::shared_ptr<channel_peer> ptr;

    /// Subscribe to messages from peer (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Message, typename Handler =
        distributor_peer::handler<Message>>
    void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        distributor_.subscribe(std::forward<Handler>(handler));
    }

    /// Serialize and write message to peer (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <class Message>
    void send(const Message& message, result_handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        using namespace messages::p2p;
        const auto id = settings().identifier;
        const auto ptr = serialize(message, id, negotiated_version());

        // Capture message in intermediate completion handler.
        auto complete = [self = shared_from_base<channel_peer>(), ptr,
            handle = std::move(handler)](const code& ec, size_t) NOEXCEPT
        {
            if (ec) self->stop(ec);
            handle(ec);
        };

        if (!ptr)
        {
            complete(error::bad_alloc, zero);
            return;
        }

        const asio::const_buffer buffer{ ptr->data(), ptr->size() };
        write(buffer, std::move(complete));
    }

    /// Construct a p2p channel to encapsulate and communicate on the socket.
    channel_peer(memory& memory, const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero) NOEXCEPT;

    /////// Pause reading from the socket, stops timers (requires strand).
    ////void pause() NOEXCEPT override;

    /// Resume reading from the socket, starts timers (requires strand).
    void resume() NOEXCEPT override;

    /// Quiet should be written only in handshake.
    /// The channel does not "speak" to peers (e.g. seed connection).
    bool quiet() const NOEXCEPT;
    void set_quiet() NOEXCEPT;

    /// Message level is supported by configured protocol level.
    bool is_negotiated(messages::p2p::level level) const NOEXCEPT;

    /// Service level is advertised by peer.
    bool is_peer_service(messages::p2p::service service) const NOEXCEPT;

    /// Start height for version message (set only before handshake).
    size_t start_height() const NOEXCEPT;
    void set_start_height(size_t height) NOEXCEPT;

    /// Negotiated version should be written only in handshake (safety).
    uint32_t negotiated_version() const NOEXCEPT;
    void set_negotiated_version(uint32_t value) NOEXCEPT;

    /// Peer version should be written only in handshake.
    messages::p2p::version::cptr peer_version() const NOEXCEPT;
    void set_peer_version(const messages::p2p::version::cptr& value) NOEXCEPT;

    /// Originating address of connection with current time and peer services.
    address_item_cptr get_updated_address() const NOEXCEPT;

protected:
    typedef messages::p2p::heading::cptr heading_ptr;

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
    // These are protected by strand/order.

    bool quiet_{};
    distributor_peer distributor_;
    uint32_t negotiated_version_;
    messages::p2p::version::cptr peer_version_{};
    size_t start_height_{};

    system::data_chunk payload_buffer_{};
    system::data_array<messages::p2p::heading::size()> heading_buffer_{};

    // Because heading buffer is fixed the stream can be reused as well.
    system::stream::in::fast heading_stream_{ heading_buffer_ };
    system::read::bytes::fast heading_reader_{ heading_stream_ };
};

} // namespace network
} // namespace libbitcoin

#endif
