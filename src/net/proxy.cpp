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
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// Dump up to 1k of payload as hex in order to diagnose failure.
static const size_t invalid_payload_dump_size = 1024;

// static
std::string proxy::extract_command(system::chunk_ptr payload)
{
    if (payload->size() < sizeof(uint32_t) + heading::command_size)
        return "<unknown>";

    return data_slice(std::next(payload->begin(), sizeof(uint32_t)),
        std::next(payload->end(), heading::command_size)).to_string();
}

proxy::proxy(socket::ptr socket)
  : socket_(socket),
    pump_subscriber_(socket->strand()),
    stop_subscriber_(std::make_shared<stop_subscriber>(socket->strand())),
    payload_buffer_(no_fill_byte_allocator),
    heading_reader_(heading_buffer_)
{
}

proxy::~proxy()
{
}

// Start/Stop.
// ----------------------------------------------------------------------------
// These calls may originate from outside the strand on any thread.

// Start the read cycle.
void proxy::start()
{
    read_heading();
}

bool proxy::stopped() const
{
    return socket_->stopped();
}

// Protocols subscribe to channel stop.
void proxy::subscribe_stop(result_handler&& handler)
{
    // Stop is posted to strand to protect socket and subscribers.
    boost::asio::post(strand(),
        std::bind(&proxy::do_subscribe,
            shared_from_this(), std::move(handler)));
}

// protected
void proxy::do_subscribe(result_handler handler)
{
    stop_subscriber_->subscribe(std::move(handler));
}

void proxy::stop(const code& ec)
{
    // Stop is posted to strand to protect socket and subscribers.
    boost::asio::post(strand(),
        std::bind(&proxy::do_stop,
            shared_from_this(), ec));
}

// protected
void proxy::do_stop(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Post message handlers to strand and clear/stop accepting subscriptions.
    // On channel_stopped message subscribers should ignore and perform no work.
    pump_subscriber_.stop(ec);

    // Post stop handlers to strand and clear/stop accepting subscriptions.
    // The code provides information on the reason that the channel stopped.
    stop_subscriber_->stop(ec);

    // Stops the read loop.
    // Signals socket to stop accepting new work, cancels pending work.
    socket_->stop();
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
    BC_ASSERT_MSG(stranded(), "strand");

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
        stop(ec);
        return;
    }

    heading_reader_.set_position(zero);
    const auto head = to_shared(heading::deserialize(heading_reader_));

    if (!heading_reader_)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid heading from [" << authority() << "]";
        stop(error::invalid_heading);
        return;
    }

    if (head->magic != protocol_magic())
    {
        // These are common, with magic 542393671 coming from http requests.
        LOG_DEBUG(LOG_NETWORK)
            << "Invalid heading magic (" << head->magic << ") from ["
            << authority() << "]";
        stop(error::invalid_magic);
        return;
    }

    if (head->payload_size > maximum_payload())
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Oversized payload indicated by " << head->command
            << " heading from [" << authority() << "] ("
            << head->payload_size << " bytes)";
        stop(error::oversized_payload);
        return;
    }

    read_payload(head);
}

void proxy::read_payload(heading_ptr head)
{
    // Buffer reserve increases with each larger message (up to maximum).
    payload_buffer_.resize(head->payload_size);

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
    BC_ASSERT_MSG(stranded(), "strand");

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
            << "Invalid " << head->command << " payload from ["
            << authority() << "] bad checksum.";
        stop(error::invalid_checksum);
        return;
    }

    // Resizable payload buffer precludes reuse of the payload reader.
    system::read::bytes::copy payload_reader(payload_buffer_);

    // Notify subscribers of the new message.
    auto code = pump_subscriber_.notify(head->id(), version(), payload_reader);

    if (code)
    {
        // TODO: consolidated with verbose log.
        if (verbose())
        {
            const auto size = std::min(payload_size, invalid_payload_dump_size);
            const auto begin = payload_buffer_.begin();

            LOG_VERBOSE(LOG_NETWORK)
                << "Invalid payload from [" << authority() << "] "
                << encode_base16({ begin, std::next(begin, size) });
        }
        else
        {
            LOG_WARNING(LOG_NETWORK)
                << "Invalid " << head->command << " payload from ["
                << authority() << "] " << code.message();
        }

        stop(code);
        return;
    }

    LOG_VERBOSE(LOG_NETWORK)
        << "Received " << head->command << " from [" << authority()
        << "] (" << payload_size << " bytes)";

    signal_activity();
    read_heading();
}

// Message send sequence.
// ----------------------------------------------------------------------------

// private
void proxy::send(system::chunk_ptr payload, result_handler&& handler)
{
    // Sends are allowed to proceed after stop, but will be aborted.

    // Post handle_send to strand upon stop, error, or buffer fully sent.
    socket_->write(*payload,
        std::bind(&proxy::handle_send,
            shared_from_this(), _1, _2, payload, std::move(handler)));
}

// private
void proxy::send(system::chunk_ptr payload, const result_handler& handler)
{
    // Sends are allowed to proceed after stop, but will be aborted.

    // Post handle_send to strand upon stop, error, or buffer fully sent.
    socket_->write(*payload,
        std::bind(&proxy::handle_send,
            shared_from_this(), _1, _2, payload, handler));
}

// private
// Handle errors and invoke completion handler.
void proxy::handle_send(const code& ec, size_t, system::chunk_ptr payload,
    const result_handler& handler)
{
    BC_ASSERT_MSG(stranded(), "strand");

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
            << "Failure sending " << extract_command(payload) << " to ["
            << authority() << "] (" << payload->size() << " bytes) "
            << ec.message();
        stop(ec);
        handler(ec);
        return;
    }

    LOG_VERBOSE(LOG_NETWORK)
        << "Sent " << extract_command(payload) << " to [" << authority()
        << "] (" << payload->size() << " bytes)";

    handler(ec);
}


// Properties.
// ----------------------------------------------------------------------------

bool proxy::stranded() const
{
    return socket_->stranded();
}

asio::strand& proxy::strand()
{
    return socket_->strand();
}

const config::authority& proxy::authority() const
{
    return socket_->authority();
}

} // namespace network
} // namespace libbitcoin
