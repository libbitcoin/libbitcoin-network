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
#ifndef LIBBITCOIN_NETWORK_PROXY_HPP
#define LIBBITCOIN_NETWORK_PROXY_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/message_subscriber.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Manages all socket communication, thread safe.
class BCT_API proxy
  : public system::enable_shared_from_base<proxy>, system::noncopyable
{
public:
    typedef std::shared_ptr<proxy> ptr;
    typedef std::function<void(const system::code&)> result_handler;
    typedef system::subscriber<system::code> stop_subscriber;

    /// Construct an instance.
    proxy(system::threadpool& pool, system::socket::ptr socket,
        const settings& settings);

    /// Validate proxy stopped.
    ~proxy();

    /// Send a message on the socket.
    template <class Message>
    void send(const Message& message, result_handler handler)
    {
        auto data = system::message::serialize(version_, message,
            protocol_magic_);
        const auto payload = std::make_shared<system::data_chunk>(
            std::move(data));
        const auto command = std::make_shared<std::string>(message.command);

        // Sequential dispatch is required because write may occur in multiple
        // asynchronous steps invoked on different threads, causing deadlocks.
        dispatch_.lock(&proxy::do_send,
            shared_from_this(), command, payload, handler);
    }

    /// Subscribe to messages of the specified type on the socket.
    template <class Message>
    void subscribe(message_handler<Message>&& handler)
    {
        message_subscriber_.subscribe<Message>(
            std::forward<message_handler<Message>>(handler));
    }

    /// Subscribe to the stop event.
    virtual void subscribe_stop(result_handler handler);

    /// Get the authority of the far end of this socket.
    virtual const system::config::authority& authority() const;

    /// Get the negotiated protocol version of this socket.
    /// The value should be the lesser of own max and peer min.
    uint32_t negotiated_version() const;

    /// Save the negotiated protocol version.
    virtual void set_negotiated_version(uint32_t value);

    /// Read messages from this socket.
    virtual void start(result_handler handler);

    /// Stop reading or sending messages on this socket.
    virtual void stop(const system::code& ec);

protected:
    virtual bool stopped() const;
    virtual void signal_activity() = 0;
    virtual void handle_stopping() = 0;

private:
    typedef system::byte_source<system::data_chunk> payload_source;
    typedef boost::iostreams::stream<payload_source> payload_stream;
    typedef std::shared_ptr<std::string> command_ptr;
    typedef std::shared_ptr<system::data_chunk> payload_ptr;

    void stop(const system::boost_code& ec);

    void read_heading();
    void handle_read_heading(const system::boost_code& ec,
        size_t payload_size);

    void read_payload(const system::message::heading& head);
    void handle_read_payload(const system::boost_code& ec, size_t,
        const system::message::heading& head);

    void do_send(command_ptr command, payload_ptr payload,
        result_handler handler);
    void handle_send(const system::boost_code& ec, size_t bytes,
        command_ptr command, payload_ptr payload, result_handler handler);

    const system::config::authority authority_;

    // These are protected by read header/payload ordering.
    system::data_chunk heading_buffer_;
    system::data_chunk payload_buffer_;
    system::socket::ptr socket_;

    // These are thread safe.
    std::atomic<bool> stopped_;
    const uint32_t protocol_magic_;
    const size_t maximum_payload_;
    const bool validate_checksum_;
    const bool verbose_;
    std::atomic<uint32_t> version_;
    message_subscriber message_subscriber_;
    stop_subscriber::ptr stop_subscriber_;
    system::dispatcher dispatch_;
};

} // namespace network
} // namespace libbitcoin

#endif

