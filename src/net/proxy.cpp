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

#include <algorithm>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace bc::system::messages;
using namespace std::placeholders;

// Dump up to 1k of payload as hex in order to diagnose failure.
static const size_t invalid_payload_dump_size = 1024;

proxy::proxy(socket::ptr socket)
  : socket_(socket),
    pump_subscriber_(socket->strand()),
    stop_subscriber_(socket->strand()),
    payload_buffer_(no_fill_byte_allocator),
    heading_buffer_(heading::maximum_size(), no_fill_byte_allocator)
{
}

// Start/Stop.
// ----------------------------------------------------------------------------

// Start the read cycle.
void proxy::start()
{
    read_heading();
}

bool proxy::stopped() const
{
    return socket_->stopped();
}

void proxy::stop(const code& ec)
{
    // Post message handlers to strand and clear/stop accepting subscriptions.
    // On channel_stopped message subscribers should ignore and perform no work.
    pump_subscriber_.stop(error::channel_stopped);

    // Post stop handlers to strand and clear/stop accepting subscriptions.
    // The code provides information on the reason that the channel stopped.
    stop_subscriber_.stop(ec);

    // Stops the read loop.
    // Signals socket to stop accepting new work, cancel pending work.
    socket_->stop();
}

bool proxy::subscribe_stop(result_handler&& handler)
{
    return stop_subscriber_.subscribe(std::move(handler));
}

// Read cycle (read continues until stop).
// ----------------------------------------------------------------------------

void proxy::read_heading()
{
    // Terminates the read loop.
    if (stopped())
        return;

    // Post handle_read_heading to strand upon stop, error, or buffer full.
    socket_->read(heading_buffer_,
        std::bind(&proxy::handle_read_heading,
            shared_from_this(), _1, _2));
}

// private
// Handle errors and invoke payload read.
void proxy::handle_read_heading(const code& ec, size_t)
{
    if (ec == error::channel_stopped)
    {
        LOG_VERBOSE(LOG_NETWORK)
            << "Heading read abort [" << authority() << "]";
        stop(error::success);
        return;
    }

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Heading read failure [" << authority() << "] " << ec.message();
        stop(error::bad_stream);
        return;
    }

    auto head = std::make_shared<heading>(heading::factory(heading_buffer_));

    if (!head->is_valid())
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid heading from [" << authority() << "]";
        stop(error::bad_stream);
        return;
    }

    if (head->magic() != protocol_magic())
    {
        // These are common, with magic 542393671 coming from http requests.
        LOG_DEBUG(LOG_NETWORK)
            << "Invalid heading magic (" << head->magic() << ") from ["
            << authority() << "]";
        stop(error::bad_stream);
        return;
    }

    if (head->payload_size() > maximum_payload())
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Oversized payload indicated by " << head->command()
            << " heading from [" << authority() << "] ("
            << head->payload_size() << " bytes)";
        stop(error::bad_stream);
        return;
    }

    read_payload(head);
}

void proxy::read_payload(heading_ptr head)
{
    // Buffer reserve increases with each larger message (up to maximum).
    payload_buffer_.resize(head->payload_size());

    // Post handle_read_payload to strand upon stop, error, or buffer full.
    socket_->read(payload_buffer_,
        std::bind(&proxy::handle_read_payload,
            shared_from_this(), _1, _2, head));
}

// private
// Handle errors and post message to subscribers.
void proxy::handle_read_payload(const code& ec, size_t payload_size,
    heading_ptr head)
{
    if (ec == error::channel_stopped)
    {
        LOG_VERBOSE(LOG_NETWORK)
            << "Payload read abort [" << authority() << "]";
        stop(error::success);
        return;
    }

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Payload read failure [" << authority() << "] "
            << ec.message();
        stop(ec);
        return;
    }

    // This is a pointless test but we allow it as an option for completeness.
    if (validate_checksum() && !head->verify_checksum(payload_buffer_))
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid " << head->command() << " payload from ["
            << authority() << "] bad checksum.";
        stop(error::bad_stream);
        return;
    }

    // Notify subscribers of the new message.
    read::bytes::copy reader(payload_buffer_);
    const auto code = pump_subscriber_.notify(head->id(), version(), reader);

    // code is limited to success or bad_stream
    if (verbose() && code)
    {
        const auto size = std::min(payload_size, invalid_payload_dump_size);
        const auto begin = payload_buffer_.begin();

        LOG_VERBOSE(LOG_NETWORK)
            << "Invalid payload from [" << authority() << "] "
            << encode_base16({ begin, std::next(begin, size) });
        stop(code);
        return;
    }

    if (code)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid " << head->command() << " payload from ["
            << authority() << "] " << code.message();
        stop(error::bad_stream);
        return;
    }

    if (!reader.is_exhausted())
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid " << head->command() << " payload from ["
            << authority() << "] trailing bytes.";
        stop(error::bad_stream);
        return;
    }

    LOG_VERBOSE(LOG_NETWORK)
        << "Received " << head->command() << " from [" << authority()
        << "] (" << payload_size << " bytes)";

    signal_activity();
    read_heading();
}

// Message send sequence.
// ----------------------------------------------------------------------------

// private
void proxy::send(command_ptr command, payload_ptr payload,
    result_handler&& handler)
{
    // Sends are allowed to proceed after stop, but will be aborted.

    // Post handle_send to strand upon stop, error, or buffer fully sent.
    socket_->write(*payload,
        std::bind(&proxy::handle_send,
            shared_from_this(), _1, _2, command, payload, std::move(handler)));
}

// private
// Handle errors and invoke completion handler.
void proxy::handle_send(const code& ec, size_t, command_ptr command,
    payload_ptr payload, const result_handler& handler)
{
    if (ec == error::channel_stopped)
    {
        LOG_VERBOSE(LOG_NETWORK)
            << "Send abort [" << authority() << "]";
        stop(error::success);
        return;
    }

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure sending " << *command << " to [" << authority()
            << "] (" << payload->size() << " bytes) " << ec.message();
        stop(ec);
        handler(ec);
        return;
    }

    LOG_VERBOSE(LOG_NETWORK)
        << "Sent " << *command << " to [" << authority() << "] ("
        << payload->size() << " bytes)";

    handler(ec);
}


// Properties.
// ----------------------------------------------------------------------------

asio::strand& proxy::strand()
{
    return socket_->strand();
}

const config::authority& proxy::authority() const
{
    return socket_->endpoint();
}

} // namespace network
} // namespace libbitcoin
