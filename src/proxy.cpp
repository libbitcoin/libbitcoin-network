/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/proxy.hpp>

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <boost/iostreams/stream.hpp>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/const_buffer.hpp>
#include <bitcoin/network/socket.hpp>

namespace libbitcoin {
namespace network {

#define NAME "proxy"

using namespace message;
using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;
using std::placeholders::_4;

// TODO: this is made up, configure payload size guard for DoS protection.
static constexpr size_t max_payload_size = 10 * 1024 * 1024;
static const auto nop = [](code){};

proxy::proxy(threadpool& pool, socket::ptr socket, uint32_t magic)
  : stopped_(true),
    magic_(magic),
    authority_(socket->get_authority()),
    socket_(socket),
    message_subscriber_(pool),
    send_subscriber_(std::make_shared<send_subscriber>(pool, NAME "_send")),
    stop_subscriber_(std::make_shared<stop_subscriber>(pool, NAME "_stop"))
{
}

proxy::~proxy()
{
    BITCOIN_ASSERT_MSG(stopped(), "The channel was not stopped.");
}

// Properties.
// ----------------------------------------------------------------------------

const config::authority& proxy::authority() const
{
    return authority_;
}

// Start sequence.
// ----------------------------------------------------------------------------

void proxy::start(result_handler handler)
{
    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    const auto sender = 
        std::bind(&proxy::do_send,
            shared_from_this(), _1, _2, _3,_4);

    stopped_ = false;
    stop_subscriber_->start();
    message_subscriber_.start();
    send_subscriber_->start();
    send_subscriber_->subscribe(sender, error::channel_stopped, {}, {}, nop);

    // Allow for subscription before first read, so no messages are missed.
    handler(error::success);

    // Start the read cycle.
    read_heading();
}

// Stop subscription.
// ----------------------------------------------------------------------------

void proxy::subscribe_stop(result_handler handler)
{
    stop_subscriber_->subscribe(handler, error::channel_stopped);
}

// Read cycle (read continues until stop).
// ----------------------------------------------------------------------------

void proxy::read_heading()
{
    if (stopped())
        return;

    // Critical Section (external)
    ///////////////////////////////////////////////////////////////////////////
    const auto socket = socket_->get_socket();

    using namespace boost::asio;
    async_read(socket->get(), buffer(heading_buffer_),
        std::bind(&proxy::handle_read_heading,
            shared_from_this(), _1, _2));
    ///////////////////////////////////////////////////////////////////////////
}

void proxy::handle_read_heading(const boost_code& ec, size_t)
{
    if (stopped())
        return;

    if (ec)
    {
        log::debug(LOG_NETWORK)
            << "Heading read failure [" << authority() << "] "
            << code(error::boost_to_error_code(ec)).message();
        stop(ec);
        return;
    }

    heading head;
    heading_stream istream(heading_buffer_);
    const auto parsed = head.from_data(istream);

    if (!parsed || head.magic != magic_)
    {
        log::warning(LOG_NETWORK) 
            << "Invalid heading from [" << authority() << "]";
        stop(error::bad_stream);
        return;
    }

    if (head.payload_size > max_payload_size)
    {
        log::warning(LOG_NETWORK)
            << "Oversized payload indicated by " << head.command
            << " heading from [" << authority() << "] ("
            << head.payload_size << " bytes)";
        stop(error::bad_stream);
        return;
    }

    ////log::debug(LOG_NETWORK)
    ////    << "Valid " << head.command << " heading from ["
    ////    << authority() << "] (" << head.payload_size << " bytes)";

    read_payload(head);
    handle_activity();
}

void proxy::read_payload(const heading& head)
{
    if (stopped())
        return;

    const auto size = head.payload_size;

    // The payload buffer is protected by ordering, not the critial section.
    payload_buffer_.resize(size);

    // Critical Section (external)
    ///////////////////////////////////////////////////////////////////////////
    const auto socket = socket_->get_socket();

    using namespace boost::asio;
    async_read(socket->get(), buffer(payload_buffer_, size),
        std::bind(&proxy::handle_read_payload,
            shared_from_this(), _1, _2, head));
    ///////////////////////////////////////////////////////////////////////////
}

void proxy::handle_read_payload(const boost_code& ec, size_t,
    const heading& head)
{
    if (stopped())
        return;

    // Ignore read error here, client may have disconnected.

    if (head.checksum != bitcoin_checksum(payload_buffer_))
    {
        log::warning(LOG_NETWORK) 
            << "Invalid " << head.command << " checksum from ["
            << authority() << "]";
        stop(error::bad_stream);
        return;
    }

    // Parse and publish the payload to message subscribers.
    payload_source source(payload_buffer_);
    payload_stream istream(source);

    // Notify subscribers of the new message.
    const auto parse_error = message_subscriber_.load(head.type(), istream);
    const auto unconsumed = istream.peek() != std::istream::traits_type::eof();

    if (stopped())
        return;

    if (!parse_error)
    {
        if (unconsumed)
            log::warning(LOG_NETWORK)
            << "Valid " << head.command << " payload from ["
                << authority() << "] unused bytes remain.";
        else
            log::debug(LOG_NETWORK)
            << "Valid " << head.command << " payload from ["
                << authority() << "] (" << payload_buffer_.size() << " bytes)";
    }

    if (ec)
    {
        log::warning(LOG_NETWORK)
            << "Payload read failure [" << authority() << "] "
            << code(error::boost_to_error_code(ec)).message();
        stop(ec);
        return;
    }

    if (parse_error)
    {
        log::warning(LOG_NETWORK)
            << "Invalid " << head.command << " stream from ["
            << authority() << "] " << parse_error.message();
        stop(parse_error);
    }

    handle_activity();
    read_heading();
}

// Message send sequence.
// ----------------------------------------------------------------------------
// vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv
// THIS REQUIRES AT LEAST TWO NETWORK THREADS TO PREVENT THREAD STARVATION.
// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

// The send subscriber pushes queued writes to the writer instead of having
// writer pull them from a passive queue. This is less thread-efficient but it
// allows us to reuse the subscriber and facilitates bypass of subscriber
// queuing for large message types such as blocks, as we do with reads.
bool proxy::do_send(const code& ec, const std::string& command,
    const_buffer buffer, result_handler handler)
{
    if (stopped())
    {
        handler(error::channel_stopped);
        return false;
    }

    if (ec)
    {
        log::debug(LOG_NETWORK)
            << "Send dequeue failure [" << authority() << "] " << ec.message();
        handler(ec);
        stop(ec);
        return false;
    }

    log::debug(LOG_NETWORK)
        << "Sending " << command << " to [" << authority() << "] ("
        << buffer.size() << " bytes)";

    // Critical Section (protect writer)
    //\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\
    // The lock must be held until the handler is invoked.
    const auto lock = std::make_shared<scope_lock>(mutex_);

    // Critical Section (protect socket)
    ///////////////////////////////////////////////////////////////////////////
    // The socket is locked until async_write returns.
    const auto socket = socket_->get_socket();

    // The shared buffer must be kept in scope until the handler is invoked.
    using namespace boost::asio;
    async_write(socket->get(), buffer,
        std::bind(&proxy::handle_send,
            shared_from_this(), _1, buffer, lock, handler));

    return true;
    ///////////////////////////////////////////////////////////////////////////
}

void proxy::handle_send(const boost_code& ec, const_buffer buffer,
    scope_lock::ptr lock, result_handler handler)
{
    lock = nullptr;
    //\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\

    const auto error = code(error::boost_to_error_code(ec));

    if (error)
        log::debug(LOG_NETWORK)
            << "Failure sending " << buffer.size() << " byte message to ["
            << authority() << "] " << error.message();

    handler(error);
}

// Stop sequence.
// ----------------------------------------------------------------------------

// This is not short-circuited by a stop test because we need to ensure it
// completes at least once before invoking the handler. This requires a unique
// lock be taken around the entire section, which poses a deadlock risk.
// Instead this is thread safe and idempotent, allowing it to be unguarded.
void proxy::stop(const code& ec)
{
    BITCOIN_ASSERT_MSG(ec, "The stop code must be an error code.");

    stopped_ = true;

    // Prevent subscription after stop.
    message_subscriber_.stop();
    message_subscriber_.broadcast(error::channel_stopped);

    // Prevent subscription after stop.
    send_subscriber_->stop();
    send_subscriber_->relay(ec, {}, {}, nop);

    // Prevent subscription after stop.
    stop_subscriber_->stop();
    stop_subscriber_->relay(ec);

    // Give channel opportunity to terminate timers.
    handle_stopping();

    // The socket_ is internally guarded against concurrent use.
    socket_->close();
}

void proxy::stop(const boost_code& ec)
{
    stop(error::boost_to_error_code(ec));
}

bool proxy::stopped() const
{
    return stopped_;
}

} // namespace network
} // namespace libbitcoin
