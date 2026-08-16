/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include <bitcoin/network/channels/channel_peer.hpp>

#include <iterator>
#include <memory>
#include <utility>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/proxy.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages::peer;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Start/stop/resume (started upon create).
// ----------------------------------------------------------------------------

// This should not be called internally.
void channel_peer::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Stops timers and any other base channel state.
    channel::stopping(ec);

    // Post message handlers to strand and clear/stop accepting subscriptions.
    // On channel_stopped message subscribers should ignore and perform no work.
    dispatcher_.stop(ec);
}

void channel_peer::resume() NOEXCEPT
{
    BC_ASSERT(stranded());
    channel::resume();
    receive();
}

// Properties.
// ----------------------------------------------------------------------------
// Version members are protected by the presumption of no reads during writes.
// Versions should only be set in handshake process, and only read thereafter.

bool channel_peer::quiet() const NOEXCEPT
{
    return quiet_;
}

void channel_peer::set_quiet() NOEXCEPT
{
    quiet_ = true;
}

bool channel_peer::is_negotiated(messages::peer::level level) const NOEXCEPT
{
    return negotiated_version() >= level;
}

bool channel_peer::is_peer_service(
    messages::peer::service service) const NOEXCEPT
{
    return to_bool(bit_and<uint64_t>(peer_version_->services, service));
}

size_t channel_peer::start_height() const NOEXCEPT
{
    return start_height_;
}

void channel_peer::set_start_height(size_t height) NOEXCEPT
{
    BC_ASSERT_MSG(!is_limited<uint32_t>(height), "Time to upgrade protocol.");
    start_height_ = height;
}

void channel_peer::set_witness(bool witness) NOEXCEPT
{
    witness_ = witness;
}

uint32_t channel_peer::negotiated_version() const NOEXCEPT
{
    return negotiated_version_;
}

void channel_peer::set_negotiated_version(uint32_t value) NOEXCEPT
{
    BC_ASSERT(stranded());
    negotiated_version_ = value;
}

bool channel_peer::current() const NOEXCEPT
{
    return current_;
}

void channel_peer::set_current(bool value) NOEXCEPT
{
    BC_ASSERT(stranded());
    current_ = value;
}

bool channel_peer::is_handshaked() const NOEXCEPT
{
    return !is_null(peer_version_);
}

// peer_version_->user_agent is not sanitized here.
version::cptr channel_peer::peer_version() const NOEXCEPT
{
    // peer_version_ defaults to nullptr, which implies not handshaked.
    return is_handshaked() ? peer_version_ : to_shared<version>();
}

void channel_peer::set_peer_version(const version::cptr& value) NOEXCEPT
{
    BC_ASSERT(stranded());
    peer_version_ = value;
}

address_item_cptr channel_peer::get_updated_address() const NOEXCEPT
{
    // Copy peer address.
    const auto peer = std::make_shared<address_item>(address());

    // Update timestamp, and services if handshaked.
    peer->timestamp = unix_time();
    if (is_handshaked())
        peer->services = peer_version_->services;

    return peer;
}

// Read cycle (read continues until stop called).
// ----------------------------------------------------------------------------

void channel_peer::receive() NOEXCEPT
{
    BC_ASSERT(stranded());

    // All prevent read loop start, reading indicates it is already started
    // (making resume idempotent), paused can be resumed, stopped cannot.
    // Pause only prevents start of the read loop, it does not prevent messages
    // from being issued for sockets already past that point (e.g. waiting).
    // This is mainly for startup coordination, preventing missed messages.
    if (stopped() || paused() || reading_)
        return;

    reading_ = true;

    // Fresh frame stamped with parse context, fault detail carried out.
    const auto in = to_shared<frame>();
    in->magic = settings().identifier;
    in->version = negotiated_version();
    in->witness = witness_;
    in->checksum = settings().validate_checksum;
    in->maximum = options().maximum_request;

    // Post handle_receive to strand upon message, stop, or error.
    read(payload_buffer_, *in,
        std::bind(&channel_peer::handle_receive,
            shared_from_base<channel_peer>(), _1, _2, in));
}

// Handle errors and post message to subscribers.
// The frame object is allocated on another thread and destroyed on this one.
// This introduces cross-thread allocation/deallocation, though size is small.
void channel_peer::handle_receive(const code& ec, size_t,
    const frame_ptr& in) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped())
    {
        LOGQ("Message read abort [" << endpoint() << "]");
        return;
    }

    if (ec)
    {
        // The frame carries parse fault detail (the read code is generic).
        const auto fault = in->fault ? in->fault : ec;
        log_fault(fault, *in);
        stop(fault);
        return;
    }

    LOGX("Recv " << in->head.command << " from [" << endpoint() << "] ("
        << in->head.payload_size << " bytes)");

    reading_ = false;

    // Notify subscribers of the new message.
    // If object passes to another thread destruction cost is very high.
    if (const auto code = dispatcher_.notify(rpc::request_t
    {
        .method = in->head.command,
        .params = { rpc::array_t{ std::move(in->payload) } }
    }))
    {
        stop(code);
        return;
    }

    // Don't retain larger than configured when current (time-space tradeoff).
    if (current() && payload_buffer_.capacity() > options().minimum_buffer)
    {
        payload_buffer_.resize(options().minimum_buffer);
        payload_buffer_.shrink_to_fit();
    }

    receive();
}

void channel_peer::handle_send(const code& ec, size_t LOG_ONLY(size),
    const std::string& LOG_ONLY(command), const result_handler& handler) NOEXCEPT
{
    if (ec)
        stop(ec);

    // Don't log common conditions.
    if (ec &&
        ec != error::peer_disconnect &&
        ec != error::operation_canceled &&
        ec != error::connect_failed)
    {
        LOGF("Send failure " << command << " to [" << endpoint() << "] ("
            << size << " bytes) " << ec.message());
    }

    handler(ec);
}

// On parse fault the frame bytes remain in the read buffer (not consumed),
// so payload diagnostics are drawn from the buffer via the parsed heading.
void channel_peer::log_fault(const code& LOG_ONLY(fault),
    const frame& LOG_ONLY(in)) const NOEXCEPT
{
#if defined(HAVE_LOGGING)
    // Dump up to this size of payload as hex in order to diagnose failure.
    static constexpr size_t invalid_payload_dump_size = 0xff;
    static constexpr uint32_t http_magic = 0x20544547;

    const auto& name = in.head.command;

    switch (fault.value())
    {
        case error::invalid_heading:
        {
            LOGR("Invalid heading from [" << endpoint() << "]");
            break;
        }
        case error::invalid_magic:
        {
            if (in.head.magic == http_magic)
            {
                LOGR("Http request from [" << endpoint() << "]");
            }
            else
            {
                LOGR("Invalid heading magic (0x"
                    << encode_base16(to_little_endian(in.head.magic))
                    << ") from [" << endpoint()
                    << "] possibly encrypted connection to clear endpoint.");
            }

            break;
        }
        case error::oversized_payload:
        {
            LOGR("Oversized payload indicated by " << name
                << " heading from [" << endpoint() << "] ("
                << in.head.payload_size << " bytes)");
            break;
        }
        case error::invalid_checksum:
        {
            LOGR("Invalid " << name << " payload from ["
                << endpoint() << "] bad checksum.");
            break;
        }
        case error::invalid_message:
        {
            LOG_ONLY(const auto start = payload_buffer_.begin();)
            LOG_ONLY(const auto end = std::next(start, std::min<size_t>(
                payload_buffer_.size(), invalid_payload_dump_size));)

            LOGR("Invalid " << name << " payload from ["
                << endpoint() << "] with bytes (" << encode_base16({ start, end })
                << "...) ");

            break;
        }
        default:
        {
            // Don't log common conditions.
            if (fault != error::peer_disconnect &&
                fault != error::operation_canceled)
            {
                LOGF("Message read failure [" << endpoint() << "] "
                    << fault.message());
            }

            break;
        }
    }
#endif
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
