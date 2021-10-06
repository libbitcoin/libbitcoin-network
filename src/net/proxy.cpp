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
#include <bitcoin/network/net/proxy.hpp>

#define BOOST_BIND_NO_PLACEHOLDERS

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "proxy"

using namespace bc::system;
using namespace bc::system::messages;
using namespace boost::asio;
using namespace std::placeholders;

// Dump up to 1k of payload as hex in order to diagnose failure.
static const size_t invalid_payload_dump_size = 1024;

// The socket owns the single thread on which this channel reads and writes.
proxy::proxy(threadpool& pool, socket::ptr socket, const settings& settings)
  : maximum_payload_(heading::maximum_payload_size(settings.protocol_maximum,
        !is_zero(settings.services & version::service::node_witness))),
    heading_buffer_(heading::maximum_size(), no_fill_byte_allocator),
    payload_buffer_(maximum_payload_, no_fill_byte_allocator),
    socket_(socket),
    stopped_(true),
    protocol_magic_(settings.identifier),
    validate_checksum_(settings.validate_checksum),
    verbose_(settings.verbose),
    version_(settings.protocol_maximum),
    authority_(socket->authority()),
    pump_(pool),
    stop_subscriber_(std::make_shared<stop_subscriber>(pool, NAME "_sub")),
    dispatch_(pool, NAME "_dispatch")
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

uint32_t proxy::negotiated_version() const
{
    return version_.load();
}

void proxy::set_negotiated_version(uint32_t value)
{
    version_.store(value);
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

    stopped_ = false;
    stop_subscriber_->start();
    pump_.start();

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

    // This will call handler upon stop, error, or buffer full.
    async_read(socket_->get(), buffer(heading_buffer_),
        std::bind(&proxy::handle_read_heading,
            shared_from_this(), _1, _2));
}

// Size does not matter because the full buffer is read if no error.
void proxy::handle_read_heading(const boost_code& ec, size_t)
{
    if (stopped())
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Heading read failure [" << authority() << "] "
            << code(error::boost_to_error_code(ec)).message();
        stop(ec);
        return;
    }

    auto head = heading::factory(heading_buffer_);

    if (!head.is_valid())
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid heading from [" << authority() << "]";
        stop(error::bad_stream);
        return;
    }

    if (head.magic() != protocol_magic_)
    {
        // These are common, with magic 542393671 coming from http requests.
        LOG_DEBUG(LOG_NETWORK)
            << "Invalid heading magic (" << head.magic() << ") from ["
            << authority() << "]";
        stop(error::bad_stream);
        return;
    }

    if (head.payload_size() > maximum_payload_)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Oversized payload indicated by " << head.command()
            << " heading from [" << authority() << "] ("
            << head.payload_size() << " bytes)";
        stop(error::bad_stream);
        return;
    }

    read_payload(std::move(head));
}

void proxy::read_payload(const heading& head)
{
    if (stopped())
        return;

    payload_buffer_.resize(head.payload_size());

    // This will call handler upon stop, error, or buffer full.
    async_read(socket_->get(), buffer(payload_buffer_),
        std::bind(&proxy::handle_read_payload,
            shared_from_this(), _1, _2, head));
}

// Size does not matter because the full buffer is read if no error.
void proxy::handle_read_payload(const boost_code& ec,
    size_t payload_size, const heading& head)
{
    if (stopped())
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Payload read failure [" << authority() << "] "
            << code(error::boost_to_error_code(ec)).message();
        stop(ec);
        return;
    }

    // This is a pointless test but we allow it as an option for completeness.
    if (validate_checksum_ && !head.verify_checksum(payload_buffer_))
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid " << head.command() << " payload from [" << authority()
            << "] bad checksum.";
        stop(error::bad_stream);
        return;
    }

    // Notify subscribers of the new message.
    read::bytes::copy reader(payload_buffer_);
    const auto code = pump_.load(head.type(), version_, reader);

    if (verbose_ && code)
    {
        const auto size = std::min(payload_size, invalid_payload_dump_size);
        const auto begin = payload_buffer_.begin();

        LOG_VERBOSE(LOG_NETWORK)
            << "Invalid payload from [" << authority() << "] "
            << encode_base16(data_chunk{ begin, begin + size });
        stop(code);
        return;
    }

    if (code)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid " << head.command() << " payload from [" << authority()
            << "] " << code.message();
        stop(code);
        return;
    }

    if (!reader.is_exhausted())
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid " << head.command() << " payload from [" << authority()
            << "] trailing bytes.";
        stop(error::bad_stream);
        return;
    }

    // Channel stopped above on failure, not forwarded to subscribers.
    LOG_VERBOSE(LOG_NETWORK)
        << "Received " << head.command() << " from [" << authority()
        << "] (" << payload_size << " bytes)";

    signal_activity();
    read_heading();
}

// Message send sequence.
// ----------------------------------------------------------------------------

// Called from send<Message> following serialization.
void proxy::send(command_ptr command, payload_ptr payload,
    result_handler handler)
{
    // Sequential dispatch is required because write may occur in multiple
    // asynchronous steps invoked on different threads.
    // boost.org/doc/libs/1_77_0/doc/html/boost_asio/reference/async_write/overload1.html
    // "This operation is implemented in terms of zero or more calls to the
    // stream's async_write_some function, and is known as a composed operation.
    // The program must ensure that the stream performs no other write operations
    // (such as async_write, the stream's async_write_some function, or any other
    // composed operations that perform writes) until this operation completes."
    // Given that async_write_some returns immediately, ordered async_write
    // execution is insufficient to preclude disorder of writes. So we queue
    // writes on this channel until the previous write has completed.
    dispatch_.lock(&proxy::do_send,
        shared_from_this(), command, payload, handler);
}

void proxy::do_send(command_ptr command, payload_ptr payload,
    result_handler handler)
{
    // This will call handler upon stop, error, or buffer fully sent.
    async_write(socket_->get(), buffer(*payload),
        std::bind(&proxy::handle_send,
            shared_from_this(), _1, _2, command, payload, handler));
}

void proxy::handle_send(const boost_code& ec, size_t,
    command_ptr command, payload_ptr payload, result_handler handler)
{
    // lock and unlock may execute on independent threads.
    dispatch_.unlock();
    const auto size = payload->size();
    const auto error = code(error::boost_to_error_code(ec));

    if (stopped())
    {
        handler(error);
        return;
    }

    if (error)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure sending " << *command << " to [" << authority()
            << "] (" << size << " bytes) " << error.message();
        stop(error);
        handler(error);
        return;
    }

    LOG_VERBOSE(LOG_NETWORK)
        << "Sent " << *command << " to [" << authority() << "] (" << size
        << " bytes)";

    handler(error);
}

// Stop sequence.
// ----------------------------------------------------------------------------

// This is not short-circuited by a stop test because we need to ensure it
// completes at least once before invoking the handler. That would require a
// lock be taken around the entire section, which poses a deadlock risk.
// Instead this is thread safe and idempotent, allowing it to be unguarded.
void proxy::stop(const code& ec)
{
    BITCOIN_ASSERT_MSG(ec, "The stop code must be an error code.");

    stopped_ = true;

    // Prevent subscription after stop.
    pump_.stop();
    pump_.broadcast(error::channel_stopped);

    // Prevent subscription after stop.
    stop_subscriber_->stop();
    stop_subscriber_->relay(ec);

    // Give channel opportunity to terminate timers.
    handle_stopping();

    // Signal socket to stop reading and accepting new work.
    socket_->stop();
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
