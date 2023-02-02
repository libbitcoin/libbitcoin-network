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
#include <functional>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// Dump up to 1k of payload as hex in order to diagnose failure.
static constexpr size_t invalid_payload_dump_size = 1024;

// This is created in a started state and must be stopped, as the subscribers
// assert if not stopped. Subscribers may hold protocols even if the service
// is not started.
proxy::proxy(const socket::ptr& socket) NOEXCEPT
  : socket_(socket),
    paused_(true),
    pump_subscriber_(socket->strand()),
    stop_subscriber_(socket->strand()),
    payload_buffer_(),
    heading_reader_(heading_buffer_),
    reporter(socket->log())
{
}

proxy::~proxy() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "proxy is not stopped");
}

// Pause (proxy is created paused).
// ----------------------------------------------------------------------------

void proxy::pause() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    paused_ = true;
}

void proxy::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    paused_ = false;
    read_heading();
}

bool proxy::paused() const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return paused_;
}

// Stop (socket/proxy is created started).
// ----------------------------------------------------------------------------

// Socket is not allowed to stop itself.
bool proxy::stopped() const NOEXCEPT
{
    return socket_->stopped();
}

void proxy::stop(const code& ec) NOEXCEPT
{
    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_stop, shared_from_this(), ec));
}

// This should not be called internally, as derived rely on stop() override.
void proxy::do_stop(const code& ec) NOEXCEPT
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
    stop_subscriber_.stop(ec);
}

void proxy::subscribe_stop(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_.subscribe(std::move(handler));
}

void proxy::subscribe_stop(result_handler&& handler,
    result_handler&& complete) NOEXCEPT
{
    boost::asio::dispatch(strand(),
        std::bind(&proxy::do_subscribe_stop,
            shared_from_this(), std::move(handler), std::move(complete)));
}

void proxy::do_subscribe_stop(const result_handler& handler,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_.subscribe(move_copy(handler));
    complete(error::success);
}

// Read cycle (read continues until stop called).
// ----------------------------------------------------------------------------

code proxy::notify(identifier id, uint32_t version,
    system::reader& source) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return pump_subscriber_.notify(id, version, source);
}

void proxy::read_heading() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Terminates the read loop (cannot be resumed).
    // Pauses the read loop (can be resumed), does not pause timer.
    if (stopped() || paused())
        return;

    // Post handle_read_heading to strand upon stop, error, or buffer full.
    socket_->read(heading_buffer_,
        std::bind(&proxy::handle_read_heading,
            shared_from_this(), _1, _2));
}

void proxy::handle_read_heading(const code& ec, size_t) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec == error::channel_stopped)
    {
        LOG("Heading read abort [" << authority() << "]");
        stop(error::success);
        return;
    }

    if (ec)
    {
        LOG("Heading read failure [" << authority() << "] " << ec.message());
        stop(ec);
        return;
    }

    heading_reader_.set_position(zero);
    const auto head = to_shared(heading::deserialize(heading_reader_));

    if (!heading_reader_)
    {
        LOG("Invalid heading from [" << authority() << "]");

        stop(error::invalid_heading);
        return;
    }

    if (head->magic != protocol_magic())
    {
        // These are common, with magic 542393671 coming from http requests.
        LOG("Invalid heading magic (" << head->magic << ") from ["
            << authority() << "]");

        stop(error::invalid_magic);
        return;
    }

    if (head->payload_size > maximum_payload())
    {
        LOG("Oversized payload indicated by " << head->command
            << " heading from [" << authority() << "] ("
            << head->payload_size << " bytes)");

        stop(error::oversized_payload);
        return;
    }

    // Buffer reserve increases with each larger message (up to maximum).
    payload_buffer_.resize(head->payload_size);

    // Post handle_read_payload to strand upon stop, error, or buffer full.
    socket_->read(payload_buffer_,
        std::bind(&proxy::handle_read_payload,
            shared_from_this(), _1, _2, head));
}

// Handle errors and post message to subscribers.
void proxy::handle_read_payload(const code& ec, size_t LOG_ONLY(payload_size),
    const heading_ptr& head) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec == error::channel_stopped)
    {
        LOG("Payload read abort [" << authority() << "]");
        stop(error::success);
        return;
    }

    if (ec)
    {
        LOG("Payload read failure [" << authority() << "] "
            << ec.message());

        stop(ec);
        return;
    }

    // This is a pointless test but we allow it as an option for completeness.
    if (validate_checksum() && !head->verify_checksum(payload_buffer_))
    {
        LOG("Invalid " << head->command << " payload from ["
            << authority() << "] bad checksum.");

        stop(error::invalid_checksum);
        return;
    }

    // Resizable payload buffer precludes reuse of the payload reader.
    system::read::bytes::copy payload_reader(payload_buffer_);

    // Notify subscribers of the new message.
    const auto code = notify(head->id(), version(), payload_reader);

    if (code)
    {
        LOGV("Invalid payload from [" << authority() << "] "
            << encode_base16(
            {
                payload_buffer_.begin(),
                std::next(payload_buffer_.begin(),
                    std::min(payload_size, invalid_payload_dump_size))
            }));

        LOG("Invalid " << head->command << " payload from ["
            << authority() << "] " << code.message());

        stop(code);
        return;
    }

    LOG("Received " << head->command << " from [" << authority()
        << "] (" << payload_size << " bytes)");

    signal_activity();
    read_heading();
}

// Send cycle (send continues until queue is empty).
// ----------------------------------------------------------------------------
// stackoverflow.com/questions/7754695/boost-asio-async-write-how-to-not-
// interleaving-async-write-calls

void proxy::write(const system::chunk_ptr& payload,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Clang no like emplace here.
    queue_.push_back(std::make_pair(payload, std::move(handler)));
    total_ = ceilinged_add(total_.load(), payload->size());
    backlog_ = ceilinged_add(backlog_.load(), payload->size());

    // Verbose.
    LOG("Queue for [" << authority() << "]: " << queue_.size()
        << " (" << backlog_.load() << " of " << total_.load() << "bytes)");

    // Start the loop if it wasn't already started.
    if (is_one(queue_.size()))
        write();
}

void proxy::write() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!queue_.empty(), "queue");

    // guarded by do_write(is_one).
    auto& job = queue_.front();

    // chunk_ptr is copied into std::bind closure to keep data alive. 
    socket_->write(*job.first,
        std::bind(&proxy::handle_write,
            shared_from_this(), _1, _2, job.first, std::move(job.second)));
}

void proxy::handle_write(const code& ec, size_t,
    const system::chunk_ptr& LOG_ONLY(payload),
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!queue_.empty(), "queue");

    // guarded by do_write(is_one).
    backlog_ = floored_subtract(backlog_.load(), queue_.front().first->size());
    queue_.pop_front();

    // Verbose.
    ////LOG("Dequeue for [" << authority() << "]: " << queue_.size()
    ////    << " (" << backlog_.load() << " bytes)");

    // All handlers must be invoked, so continue regardless of error state.
    // Handlers are invoked in queued order, after all outstanding complete.
    if (!queue_.empty())
        write();

    if (ec == error::channel_stopped)
    {
        LOG("Send abort [" << authority() << "]");
        stop(error::success);
        return;
    }

    if (ec)
    {
        LOG("Failure sending " << extract_command(*payload) << " to ["
            << authority() << "] (" << payload->size() << " bytes) "
            << ec.message());

        stop(ec);
        handler(ec);
        return;
    }

    LOG("Sent " << extract_command(*payload) << " to [" << authority()
        << "] (" << payload->size() << " bytes)");

    handler(ec);
}

// static
std::string proxy::extract_command(const system::data_chunk& payload) NOEXCEPT
{
    if (payload.size() < sizeof(uint32_t) + heading::command_size)
        return "<unknown>";

    std::string out{};
    auto at = std::next(payload.begin(), sizeof(uint32_t));
    const auto end = std::next(at, heading::command_size);
    while (at != end && *at != 0x00) out.push_back(*at++);
    return out;
}

// Properties.
// ----------------------------------------------------------------------------

asio::strand& proxy::strand() NOEXCEPT
{
    return socket_->strand();
}

bool proxy::stranded() const NOEXCEPT
{
    return socket_->stranded();
}

// TODO: test.
uint64_t proxy::backlog() const NOEXCEPT
{
    return backlog_.load(std::memory_order_relaxed);
}

// TODO: test.
uint64_t proxy::total() const NOEXCEPT
{
    return total_.load(std::memory_order_relaxed);
}

const config::authority& proxy::authority() const NOEXCEPT
{
    return socket_->authority();
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
