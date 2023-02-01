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
#ifndef LIBBITCOIN_NETWORK_NET_PROXY_HPP
#define LIBBITCOIN_NETWORK_NET_PROXY_HPP

#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/pump.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

/// Abstract, thread safe except for:
/// * pause/resume/paused/ must be called from channel strand.
/// * subscribe/subscribe_stop must be called from channel strand.
/// notify/send_bytes are protected/virtual for test access only.
/// Handles all channel communication, error handling, and logging.
class BCT_API proxy
  : public enable_shared_from_base<proxy>, public reporter
{
public:
    DELETE_COPY_MOVE(proxy);
    virtual ~proxy() NOEXCEPT;

    typedef std::shared_ptr<proxy> ptr;
    typedef subscriber<const code&> stop_subscriber;
    typedef std::function<void(const code&)> result_handler;

    /// Send a message to the peer.
    template <class Message>
    void send(const Message& message, result_handler&& complete) NOEXCEPT
    {
        // Serializes message to a chunk pointer for type-agnostic send.
        send_bytes(messages::serialize(message, protocol_magic(), version()),
            std::move(complete));
    }

    /// Subscribe to messages from peer (requires strand).
    template <class Message, typename Handler = pump::handler<Message>>
        void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        pump_subscriber_.subscribe(std::forward<Handler>(handler));
    }

    /// Pause reading from the socket (requires strand).
    virtual void pause() NOEXCEPT;

    /// Resume reading from the socket (requires strand).
    virtual void resume() NOEXCEPT;

    /// Reading from the socket is paused (requires strand).
    bool paused() const NOEXCEPT;

    /// Subscribe to stop notification with completion handler.
    void subscribe_stop(result_handler&& handler,
        result_handler&& complete) NOEXCEPT;

    /// Idempotent, may be called multiple times.
    virtual void stop(const code& ec) NOEXCEPT;

    /// The channel strand.
    asio::strand& strand() NOEXCEPT;

    /// The strand is running in this thread.
    bool stranded() const NOEXCEPT;

    /// The proxy (socket) is stopped.
    bool stopped() const NOEXCEPT;

    /// The authority of the peer.
    const config::authority& authority() const NOEXCEPT;

protected:
    /// Extract message command name from a payload.
    static std::string extract_command(
        const system::data_chunk& payload) NOEXCEPT;

    proxy(const socket::ptr& socket) NOEXCEPT;

    /// Property values provided to the proxy.
    virtual size_t maximum_payload() const NOEXCEPT = 0;
    virtual uint32_t protocol_magic() const NOEXCEPT = 0;
    virtual bool validate_checksum() const NOEXCEPT = 0;
    virtual uint32_t version() const NOEXCEPT = 0;
    virtual void signal_activity() NOEXCEPT = 0;

    /// Send bytes to the peer.
    virtual void send_bytes(const system::chunk_ptr& payload,
        result_handler&& handler) NOEXCEPT;

    /// Notify subscribers of a new message (requires strand).
    virtual code notify(messages::identifier id, uint32_t version,
        system::reader& source) NOEXCEPT;

    /// Subscribe to stop notification (requires strand).
    void subscribe_stop(result_handler&& handler) NOEXCEPT;

private:
    typedef messages::heading::ptr heading_ptr;

    void do_stop(const code& ec) NOEXCEPT;
    void do_subscribe_stop(const result_handler& handler,
        const result_handler& complete) NOEXCEPT;

    void read_heading() NOEXCEPT;
    void handle_read_heading(const code& ec, size_t heading_size) NOEXCEPT;
    void handle_read_payload(const code& ec, size_t payload_size,
        const heading_ptr& head) NOEXCEPT;
    void handle_send(const code& ec, size_t bytes,
        const system::chunk_ptr& payload,
        const result_handler& handler) NOEXCEPT;

    // This is thread safe.
    socket::ptr socket_;

    // This is not thread safe.
    bool paused_;

    // These are protected by the strand.
    pump pump_subscriber_;
    stop_subscriber stop_subscriber_;

    // These are protected by read header/payload ordering (strand).
    system::data_chunk payload_buffer_;
    system::data_array<messages::heading::size()> heading_buffer_;
    system::read::bytes::copy heading_reader_;
};

} // namespace network
} // namespace libbitcoin

#endif
