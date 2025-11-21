/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

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
    BC_ASSERT_MSG(stranded(), "strand");

    // Stops timers and any other base channel state.
    channel::stopping(ec);

    // Post message handlers to strand and clear/stop accepting subscriptions.
    // On channel_stopped message subscribers should ignore and perform no work.
    dispatcher_.stop(ec);
}

// TODO: resume of an idle channel results in termination for invalid_magic.
void channel_peer::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    channel::resume();
    read_heading();
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

bool channel_peer::is_peer_service(messages::peer::service service) const NOEXCEPT
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

uint32_t channel_peer::negotiated_version() const NOEXCEPT
{
    return negotiated_version_;
}

void channel_peer::set_negotiated_version(uint32_t value) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    negotiated_version_ = value;
}

bool channel_peer::is_handshaked() const NOEXCEPT
{
    return !is_null(peer_version_);
}

version::cptr channel_peer::peer_version() const NOEXCEPT
{
    // peer_version_ defaults to nullptr, which implies not handshaked.
    return is_handshaked() ? peer_version_ : to_shared<messages::peer::version>();
}

void channel_peer::set_peer_version(const version::cptr& value) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
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

void channel_peer::read_heading() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Both terminate read loop, paused can be resumed, stopped cannot.
    // Pause only prevents start of the read loop, it does not prevent messages
    // from being issued for sockets already past that point (e.g. waiting).
    // This is mainly for startup coordination, preventing missed messages.
    if (stopped() || paused())
        return;

    // Post handle_read_heading to strand upon stop, error, or buffer full.
    read({ heading_buffer_.data(), heading_buffer_.size() },
        std::bind(&channel_peer::handle_read_heading,
            shared_from_base<channel_peer>(), _1, _2));
}

void channel_peer::handle_read_heading(const code& ec, size_t) NOEXCEPT
{
    static constexpr uint32_t http_magic = 0x20544547;
    static constexpr uint32_t https_magic = 0x02010316;

    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Heading read abort [" << authority() << "]");
        return;
    }

    if (ec)
    {
        // Don't log common conditions.
        if (ec != error::peer_disconnect && ec != error::operation_canceled)
        {
            LOGF("Heading read failure [" << authority() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    heading_reader_.set_position(zero);
    const auto head = to_shared(heading::deserialize(heading_reader_));

    if (!heading_reader_)
    {
        LOGR("Invalid heading from [" << authority() << "]");
        stop(error::invalid_heading);
        return;
    }

    if (head->magic != settings().identifier)
    {
        if (head->magic == http_magic || head->magic == https_magic)
        {
            LOGR("Http/s request from [" << authority() << "]");
        }
        else
        {
            LOGR("Invalid heading magic (0x"
                << encode_base16(to_little_endian(head->magic))
                << ") from [" << authority() << "]");
        }

        stop(error::invalid_magic);
        return;
    }

    if (head->payload_size > settings().maximum_payload())
    {
        LOGR("Oversized payload indicated by " << head->command
            << " heading from [" << authority() << "] ("
            << head->payload_size << " bytes)");

        stop(error::oversized_payload);
        return;
    }

    // Buffer capacity increases with each larger message (up to maximum).
    payload_buffer_.resize(head->payload_size);

    // Post handle_read_payload to strand upon stop, error, or buffer full.
    read({ payload_buffer_.data(), payload_buffer_.size() },
        std::bind(&channel_peer::handle_read_payload,
            shared_from_base<channel_peer>(), _1, _2, head));
}

// Handle errors and post message to subscribers.
// The head object is allocated on another thread and destroyed on this one.
// This introduces cross-thread allocation/deallocation, though size is small.
void channel_peer::handle_read_payload(const code& ec, size_t payload_size,
    const heading_ptr& head) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Payload read abort [" << authority() << "]");
        return;
    }

    if (ec)
    {
        // Don't log common conditions.
        if (ec != error::peer_disconnect && ec != error::operation_canceled)
        {
            LOGF("Payload read failure [" << authority() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    if (settings().validate_checksum)
    {
        // This hash could be reused as w/txid, but simpler to disable check.
        if (head->checksum != network_checksum(bitcoin_hash(payload_buffer_)))
        {
            LOGR("Invalid " << head->command << " payload from ["
                << authority() << "] bad checksum.");

            stop(error::invalid_checksum);
            return;
        }
    }

    // Notify subscribers of the new message.
    ///////////////////////////////////////////////////////////////////////////
    // TODO: hack, move into peer::body::reader.
    if (auto any = interface::deserialize(allocator_, head->id(),
        payload_buffer_, negotiated_version(), settings().witness_node()))
    {
        // If object passes to another thread destruction cost is very high.
        if (const auto code = dispatcher_.notify(rpc::request_t
        {
            .method = head->command,
            .params = { rpc::array_t{ std::move(any) } }
        }))
        {
            stop(code);
            return;
        }
    }
    else
    {
        log_message(head->command, payload_size);
        stop(error::invalid_message);
        return;
    }
    ///////////////////////////////////////////////////////////////////////////

    // Don't retain larger than configured (time-space tradeoff).
    if (settings().minimum_buffer < payload_buffer_.capacity())
    {
        payload_buffer_.resize(settings().minimum_buffer);
        payload_buffer_.shrink_to_fit();
    }

    LOGX("Recv " << head->command << " from [" << authority() << "] ("
        << payload_size << " bytes)");

    read_heading();
}

void channel_peer::handle_send(const code& ec, size_t,
    const system::chunk_cptr&, const result_handler& handler) NOEXCEPT
{
    if (ec)
        stop(ec);

    handler(ec);
}

void channel_peer::log_message(const std::string_view& name,
    size_t LOG_ONLY(size)) const NOEXCEPT
{
    // Dump up to this size of payload as hex in order to diagnose failure.
    static constexpr size_t invalid_payload_dump_size = 0xff;

    if (name == messages::peer::transaction::command ||
        name == messages::peer::block::command)
    {
        LOGR("Invalid " << name << " payload from ["
            << authority() << "] with hash ["
            << encode_hash(bitcoin_hash(payload_buffer_)) << "] ");
    }
    else
    {
        LOGR("Invalid " << name << " payload from ["
            << authority() << "] with bytes (" << encode_base16(
                {
                    payload_buffer_.begin(),
                    std::next(payload_buffer_.begin(),
                    std::min(size, invalid_payload_dump_size))
                })
            << "...) ");
    }
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
