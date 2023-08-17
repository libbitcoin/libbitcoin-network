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
#ifndef LIBBITCOIN_NETWORK_NET_PROXY_HPP
#define LIBBITCOIN_NETWORK_NET_PROXY_HPP

#include <atomic>
#include <deque>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/distributor.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

/// Abstract, thread safe except some methods requiring strand.
/// Handles all channel communication, error handling, and logging.
class BCT_API proxy
  : public enable_shared_from_base<proxy>, public reporter
{
public:
    typedef std::shared_ptr<proxy> ptr;
    typedef subscriber<> stop_subscriber;

    DELETE_COPY_MOVE(proxy);

    /// Serialize and write a message to the peer (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <class Message>
    void send(const Message& message, result_handler&& complete) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        // TODO: build witness into feature w/magic and negotiated version.
        // TODO: if self and peer services show witness, set feature true.
        const auto data = messages::serialize(message, protocol_magic(),
            version());

        if (!data)
        {
            // This is an internal error, should never happen.
            LOGF("Serialization failure (" << Message::command << ").");
            complete(error::unknown);
            return;
        }

        write(data, std::move(complete));
    }

    /// Subscribe to messages from peer (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Message, typename Handler = distributor::handler<Message>>
    void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        distributor_.subscribe(std::forward<Handler>(handler));
    }

    /// Asserts/logs stopped.
    virtual ~proxy() NOEXCEPT;

    /// Pause reading from the socket (requires strand).
    virtual void pause() NOEXCEPT;

    /// Resume reading from the socket (requires strand).
    virtual void resume() NOEXCEPT;

    /// Reading from the socket is paused (requires strand).
    virtual bool paused() const NOEXCEPT;

    /// Idempotent, may be called multiple times.
    virtual void stop(const code& ec) NOEXCEPT;

    /// Subscribe to stop notification with completion handler.
    /// Completion and event handlers are always invoked on the channel strand.
    void subscribe_stop(result_handler&& handler,
        result_handler&& complete) NOEXCEPT;

    /// The channel strand.
    asio::strand& strand() NOEXCEPT;

    /// The strand is running in this thread.
    bool stranded() const NOEXCEPT;

    /// The proxy (socket) is stopped.
    bool stopped() const NOEXCEPT;

    /// The number of bytes in the write backlog.
    uint64_t backlog() const NOEXCEPT;

    /// The total number of bytes queued/sent to the peer.
    uint64_t total() const NOEXCEPT;

    /// The socket was accepted (vs. connected).
    bool inbound() const NOEXCEPT;

    /// Get the authority (incoming) of the remote endpoint.
    const config::authority& authority() const NOEXCEPT;

    /// Get the address (outgoing) of the remote endpoint.
    const config::address& address() const NOEXCEPT;

protected:
    proxy(const socket::ptr& socket) NOEXCEPT;

    /// Property values provided to the proxy.
    virtual size_t minimum_buffer() const NOEXCEPT = 0;
    virtual size_t maximum_payload() const NOEXCEPT = 0;
    virtual uint32_t protocol_magic() const NOEXCEPT = 0;
    virtual bool validate_checksum() const NOEXCEPT = 0;
    virtual uint32_t version() const NOEXCEPT = 0;

    /// Events provided by the proxy.

    /// A message has been received from the peer.
    virtual void signal_activity() NOEXCEPT = 0;

    /// Notify subscribers of a new message (requires strand).
    virtual code notify(messages::identifier id, uint32_t version,
        const system::data_chunk& source) NOEXCEPT;

    /// Send a serialized message to the peer.
    virtual void write(const system::chunk_ptr& payload,
        const result_handler& handler) NOEXCEPT;

    /// Subscribe to stop notification (requires strand).
    void subscribe_stop(result_handler&& handler) NOEXCEPT;

private:
    typedef messages::heading::cptr heading_ptr;
    typedef std::deque<std::pair<system::chunk_ptr, result_handler>> queue;

    void do_stop(const code& ec) NOEXCEPT;
    void do_subscribe_stop(const result_handler& handler,
        const result_handler& complete) NOEXCEPT;

    void read_heading() NOEXCEPT;
    void handle_read_heading(const code& ec, size_t heading_size) NOEXCEPT;
    void handle_read_payload(const code& ec, size_t payload_size,
        const heading_ptr& head) NOEXCEPT;

    void write() NOEXCEPT;
    void handle_write(const code& ec, size_t bytes,
        const system::chunk_ptr& payload,
        const result_handler& handler) NOEXCEPT;

    // These are thread safe.
    std::atomic_bool paused_{ true };
    std::atomic<uint64_t> backlog_{};
    std::atomic<uint64_t> total_{};
    socket::ptr socket_;

    // These are protected by strand.
    queue queue_{};
    system::data_chunk payload_buffer_{};
    system::data_array<messages::heading::size()> heading_buffer_{};
    system::read::bytes::copy heading_reader_{ heading_buffer_ };
    stop_subscriber stop_subscriber_;
    distributor distributor_;
};

} // namespace network
} // namespace libbitcoin

#endif
