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
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// Dump up to 1k of payload as hex in order to diagnose failure.
static constexpr size_t invalid_payload_dump_size = 1024;

// This is created in a started state and must be stopped, as the subscribers
// assert if not stopped. Subscribers may hold protocols even if the service
// is not started.
proxy::proxy(const socket::ptr& socket) noexcept
  : socket_(socket),
    paused_(true),
    pump_subscriber_(socket->strand()),
    stop_subscriber_(std::make_shared<stop_subscriber>(socket->strand())),
    payload_buffer_(),
    heading_reader_(heading_buffer_)
{
}

proxy::~proxy() noexcept
{
    BC_ASSERT_MSG(stopped(), "proxy is not stopped");
}

// Pause (proxy is created paused).
// ----------------------------------------------------------------------------

void proxy::pause() noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    paused_ = true;
}

void proxy::resume() noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    paused_ = false;
    read_heading();
}

bool proxy::paused() const noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    return paused_;
}

// Stop (socket/proxy is created started).
// ----------------------------------------------------------------------------

// Socket is not allowed to stop itself.
bool proxy::stopped() const noexcept
{
    return socket_->stopped();
}

void proxy::stop(const code& ec) noexcept
{
    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_stop, shared_from_this(), ec));
}

// This should not be called internally, as derived rely on stop() override.
void proxy::do_stop(const code& ec) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    // Stops the read loop.
    // Signals socket to stop accepting new work, cancels pending work.
    socket_->stop();

    // Overruled by stop, set only for consistency.
    paused_ = true;

    // Post message handlers to strand and clear/stop accepting subscriptions.
    // On channel_stopped message subscribers should ignore and perform no work.
    pump_subscriber_.stop(ec);

    // Post stop handlers to strand and clear/stop accepting subscriptions.
    // The code provides information on the reason that the channel stopped.
    stop_subscriber_->stop(ec);
}

void proxy::subscribe_stop(result_handler&& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_->subscribe(move_copy(handler));
}

void proxy::subscribe_stop(result_handler&& handler,
    result_handler&& complete) noexcept
{
    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_subscribe_stop,
            shared_from_this(), std::move(handler), std::move(complete)));
}

void proxy::do_subscribe_stop(const result_handler& handler,
    const result_handler& complete) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_->subscribe(move_copy(handler));
    complete(error::success);
}

// Read cycle (read continues until stop called).
// ----------------------------------------------------------------------------

// TODO: change integer version() to active() structure.
code proxy::notify(identifier id, uint32_t version,
    system::reader& source) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");
    return pump_subscriber_.notify(id, version, source);
}

void proxy::read_heading() noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates the read loop (cannot be resumed).
    if (stopped())
        return;

    // Pauses the read loop (can be resumed), does not pause timer.
    if (paused())
        return;

    // Post handle_read_heading to strand upon stop, error, or buffer full.
    socket_->read(heading_buffer_,
        std::bind(&proxy::handle_read_heading,
            shared_from_this(), _1, _2));
}

void proxy::handle_read_heading(const code& ec, size_t) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec == error::channel_stopped)
    {
        LOG_VERBOSE(LOG_NETWORK)
            << "Heading read abort [" << authority() << "]" << std::endl;
        stop(error::success);
        return;
    }

    if (ec)
    {
        ////LOG_DEBUG(LOG_NETWORK)
        ////    << "Heading read failure [" << authority() << "] " << ec.message()
        ////    << std::endl;
        stop(ec);
        return;
    }

    heading_reader_.set_position(zero);
    const auto head = to_shared(heading::deserialize(heading_reader_));

    if (!heading_reader_)
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid heading from [" << authority() << "]" << std::endl;
        stop(error::invalid_heading);
        return;
    }

    if (head->magic != protocol_magic())
    {
        // These are common, with magic 542393671 coming from http requests.
        LOG_DEBUG(LOG_NETWORK)
            << "Invalid heading magic (" << head->magic << ") from ["
            << authority() << "]" << std::endl;
        stop(error::invalid_magic);
        return;
    }

    if (head->payload_size > maximum_payload())
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Oversized payload indicated by " << head->command
            << " heading from [" << authority() << "] ("
            << head->payload_size << " bytes)" << std::endl;
        stop(error::oversized_payload);
        return;
    }

    // TODO: shrink buffer on some event.
    // Buffer reserve increases with each larger message (up to maximum).
    payload_buffer_.resize(head->payload_size);

    // Post handle_read_payload to strand upon stop, error, or buffer full.
    socket_->read(payload_buffer_,
        std::bind(&proxy::handle_read_payload,
            shared_from_this(), _1, _2, head));
}

// Handle errors and post message to subscribers.
void proxy::handle_read_payload(const code& ec, size_t payload_size,
    const heading_ptr& head) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec == error::channel_stopped)
    {
        LOG_VERBOSE(LOG_NETWORK)
            << "Payload read abort [" << authority() << "]" << std::endl;
        stop(error::success);
        return;
    }

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Payload read failure [" << authority() << "] "
            << ec.message() << std::endl;
        stop(ec);
        return;
    }

    // This is a pointless test but we allow it as an option for completeness.
    if (validate_checksum() && !head->verify_checksum(payload_buffer_))
    {
        LOG_WARNING(LOG_NETWORK)
            << "Invalid " << head->command << " payload from ["
            << authority() << "] bad checksum." << std::endl;
        stop(error::invalid_checksum);
        return;
    }

    // Resizable payload buffer precludes reuse of the payload reader.
    system::read::bytes::copy payload_reader(payload_buffer_);

    // TODO: change integer version() to active() structure.
    // Notify subscribers of the new message.
    const auto code = notify(head->id(), version(), payload_reader);

    if (code)
    {
        // TODO: consolidated with verbose log.
        if (verbose())
        {
            const auto size = std::min(payload_size, invalid_payload_dump_size);
            const auto begin = payload_buffer_.begin();

            LOG_VERBOSE(LOG_NETWORK)
                << "Invalid payload from [" << authority() << "] "
                << encode_base16({ begin, std::next(begin, size) })
                << std::endl;
        }
        else
        {
            LOG_WARNING(LOG_NETWORK)
                << "Invalid " << head->command << " payload from ["
                << authority() << "] " << code.message() << std::endl;
        }

        stop(code);
        return;
    }

    LOG_VERBOSE(LOG_NETWORK)
        << "Received " << head->command << " from [" << authority()
        << "] (" << payload_size << " bytes)" << std::endl;

    signal_activity();
    read_heading();
}

// Message send sequence.
// ----------------------------------------------------------------------------

void proxy::send_bytes(const system::chunk_ptr& payload,
    result_handler&& handler) noexcept
{
    // chunk_ptr is copied into std::bind closure. 
    // Post handle_send to strand upon stop, error, or buffer fully sent.
    socket_->write(*payload,
        std::bind(&proxy::handle_send,
            shared_from_this(), _1, _2, payload, std::move(handler)));
}

// static
std::string proxy::extract_command(const system::chunk_ptr& payload) noexcept
{
    if (payload->size() < sizeof(uint32_t) + heading::command_size)
        return "<unknown>";

    std::string out;
    auto at = std::next(payload->begin(), sizeof(uint32_t));
    const auto end = std::next(at, heading::command_size);
    while (at != end && *at != 0x00)
        out.push_back(*at++);

    return out;
}

void proxy::handle_send(const code& ec, size_t,
    const system::chunk_ptr& payload, const result_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec == error::channel_stopped)
    {
        LOG_VERBOSE(LOG_NETWORK)
            << "Send abort [" << authority() << "]" << std::endl;
        stop(error::success);
        return;
    }

    if (ec)
    {
        ////LOG_DEBUG(LOG_NETWORK)
        ////    << "Failure sending " << extract_command(payload) << " to ["
        ////    << authority() << "] (" << payload->size() << " bytes) "
        ////    << ec.message() << std::endl;
        stop(ec);
        handler(ec);
        return;
    }

    LOG_VERBOSE(LOG_NETWORK)
        << "Sent " << extract_command(payload) << " to [" << authority()
        << "] (" << payload->size() << " bytes)" << std::endl;

    handler(ec);
}

// Properties.
// ----------------------------------------------------------------------------

asio::strand& proxy::strand() noexcept
{
    return socket_->strand();
}

bool proxy::stranded() const noexcept
{
    return socket_->stranded();
}

const config::authority& proxy::authority() const noexcept
{
    return socket_->authority();
}

} // namespace network
} // namespace libbitcoin
