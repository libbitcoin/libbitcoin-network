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

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/pump.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

/// Virtual base for all channel communication error handling and logging.
/// This class is thread safe, though start my be called only once.
/// Stop is thread safe and idempotent, may be called multiple times.
/// All handlers are posted to the socket strand.
class BCT_API proxy
  : public enable_shared_from_base<proxy>, system::noncopyable
{
public:
    typedef std::shared_ptr<proxy> ptr;
    typedef subscriber<asio::strand, code> stop_subscriber;
    typedef std::function<void(const code&)> result_handler;

    // Construction.
    // ------------------------------------------------------------------------

    /// Construct an instance from a socket.
    proxy(socket::ptr socket);

    // Start/Stop.
    // ------------------------------------------------------------------------

    /// Start reading messages from the socket (call only once).
    /// Messages are posted to each subscribed pump::handler.
    virtual void start();

    /// Subscribe to proxy stop notifications.
    virtual bool subscribe_stop(result_handler&& handler);

    /// Stop has been signaled, work is stopping.
    virtual bool stopped() const;

    /// Cancel work and close the socket (idempotent).
    /// This action is deferred to the strand, not immediately affected.
    /// Block on threadpool.join() to ensure termination of the connection.
    /// Code is passed to stop subscribers, channel_stopped to message pump. 
    virtual void stop(const code& ec);

    // I/O.
    // ------------------------------------------------------------------------

    // TODO: Avoid reserializing the same object for every peer, such as by
    // TODO: broadcast. Have the object maintain a shared copy of its
    // TODO: serialization (mapped to version to the extent that it varies
    // TODO: by version). Obtain the shared pointer here by version and
    // TODO: send it. See also comments in pump::do_notify.

    /// Send a message on the socket, does not require proxy to be started.
    /// Message is serialized and does not have to be retained by the caller.
    template <class Message>
    void send(const Message& message, result_handler&& handler)
    {
        send(messages::serialize(message, protocol_magic(), version()),
            std::move(handler));
    }

    /// Subscribe to messages of type Message received by the started socket.
    /// Subscription handler is copied and retained in the queue until stop.
    template <class Message, typename Handler = pump::handler<Message>>
    bool subscribe(Handler&& handler)
    {
        return pump_subscriber_.subscribe(std::forward<Handler>(handler));
    }

    // Properties.
    // ------------------------------------------------------------------------

    /// Get the strand of the socket.
    virtual asio::strand& strand();

    /// Get the authority of the peer.
    virtual config::authority authority() const;

private:
    typedef chunk_ptr payload_ptr;
    typedef messages::heading::ptr heading_ptr;

    static std::string extract_command(payload_ptr payload);

    virtual size_t maximum_payload() const = 0;
    virtual uint32_t protocol_magic() const = 0;
    virtual bool validate_checksum() const = 0;
    virtual bool verbose() const = 0;
    virtual uint32_t version() const = 0;
    virtual void signal_activity() = 0;

    void read_heading();
    void handle_read_heading(const code& ec, size_t heading_size);

    void read_payload(heading_ptr head);
    void handle_read_payload(const code& ec, size_t payload_size,
        heading_ptr head);

    void send(payload_ptr payload, result_handler&& handler);
    void handle_send(const code& ec, size_t bytes, payload_ptr payload,
        const result_handler& handler);

    // These are thread safe.
    socket::ptr socket_;
    pump pump_subscriber_;
    stop_subscriber stop_subscriber_;

    // These are protected by read header/payload ordering.
    system::data_chunk payload_buffer_;
    system::data_array<messages::heading::size()> heading_buffer_;
    system::read::bytes::copy heading_reader_;
};

} // namespace network
} // namespace libbitcoin

#endif
