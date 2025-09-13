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
#include <bitcoin/network/net/channel_peer.hpp>

#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages::p2p;
using namespace std::placeholders;

// Dump up to this size of payload as hex in order to diagnose failure.
static constexpr size_t invalid_payload_dump_size = 0xff;
static constexpr uint32_t http_magic = 0x20544547;
static constexpr uint32_t https_magic = 0x02010316;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Factory for fixed deadline timer pointer construction.
inline deadline::ptr timeout(const logger& log, asio::strand& strand,
    const deadline::duration& span) NOEXCEPT
{
    return std::make_shared<deadline>(log, strand, span);
}

// Factory for varied deadline timer pointer construction.
inline deadline::ptr expiration(const logger& log, asio::strand& strand,
    const deadline::duration& span) NOEXCEPT
{
    return timeout(log, strand, pseudo_random::duration(span));
}

channel_peer::channel_peer(memory& memory, const logger& log,
    const socket::ptr& socket, const network::settings& settings,
    uint64_t identifier) NOEXCEPT
  : channel(log, socket, settings, identifier),
    distributor_(memory, socket->strand()),
    expiration_(expiration(log, socket->strand(), settings.channel_expiration())),
    inactivity_(timeout(log, socket->strand(), settings.channel_inactivity())),
    negotiated_version_(settings.protocol_maximum),
    tracker<channel_peer>(log)
{
}

// Stop (started upon create).
// ----------------------------------------------------------------------------

void channel_peer::stop(const code& ec) NOEXCEPT
{
    // Stop the read loop, stop accepting new work, cancel pending work.
    channel::stop(ec);

    // Stop is posted to strand to protect timers.
    boost::asio::post(strand(),
        std::bind(&channel_peer::do_stop,
            shared_from_base<channel_peer>(), ec));
}

// This should not be called internally, as derived rely on stop() override.
void channel_peer::do_stop(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_expiration();
    stop_inactivity();

    // TODO: this was moved from proxy::do_stop, so order of stop has changed.
    // Post message handlers to strand and clear/stop accepting subscriptions.
    // On channel_stopped message subscribers should ignore and perform no work.
    distributor_.stop(ec);
}

// Pause/resume (paused upon create).
// ----------------------------------------------------------------------------

// Timers are set for handshake and reset upon protocol start.
// Version protocols may have more restrictive completion timeouts.
// A restarted timer invokes completion handler with error::operation_canceled.

void channel_peer::pause() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_expiration();
    stop_inactivity();
    channel::pause();
}

void channel_peer::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    start_expiration();
    start_inactivity();

    // Resume from pause and then start read loop.
    channel::resume();

    // TODO: resume of an idle channel results in termination for invalid_magic.
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

bool channel_peer::is_negotiated(messages::p2p::level level) const NOEXCEPT
{
    return negotiated_version() >= level;
}

bool channel_peer::is_peer_service(messages::p2p::service service) const NOEXCEPT
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

// private
bool channel_peer::is_handshaked() const NOEXCEPT
{
    return !is_null(peer_version_);
}

version::cptr channel_peer::peer_version() const NOEXCEPT
{
    // peer_version_ defaults to nullptr, which implies not handshaked.
    return is_handshaked() ? peer_version_ : to_shared<messages::p2p::version>();
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

// Timers.
// ----------------------------------------------------------------------------
// TODO: build DoS protection around rate_limit_, backlog(), total(), and time.
// A restarted timer invokes completion handler with error::operation_canceled.
// Called from start or strand.

void channel_peer::stop_expiration() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    expiration_->stop();
}

void channel_peer::start_expiration() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    // Handler is posted to the socket strand.
    expiration_->start(
        std::bind(&channel_peer::handle_expiration,
            shared_from_base<channel_peer>(), _1));
}

void channel_peer::handle_expiration(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // error::operation_canceled is set by timer reset (channel not stopped).
    if (stopped() || ec == error::operation_canceled)
        return;

    if (ec)
    {
        LOGF("Lifetime timer fail [" << authority() << "] " << ec.message());
        stop(ec);
        return;
    }

    stop(error::channel_expired);
}

void channel_peer::stop_inactivity() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    inactivity_->stop();
}

// Cancels previous timer and retains configured duration.
void channel_peer::start_inactivity() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
        return;

    // Handler is posted to the socket strand.
    inactivity_->start(
        std::bind(&channel_peer::handle_inactivity,
            shared_from_base<channel_peer>(), _1));
}

// There is no timeout set on individual sends and receives, just inactivity.
void channel_peer::handle_inactivity(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // error::operation_canceled is set by timer reset (channel not stopped).
    if (stopped() || ec == error::operation_canceled)
        return;

    if (ec)
    {
        LOGF("Inactivity timer fail [" << authority() << "] " << ec.message());
        stop(ec);
        return;
    }

    stop(error::channel_inactive);
}

code channel_peer::notify(messages::p2p::identifier id, uint32_t version,
    const data_chunk& source) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // TODO: build witness into feature w/magic and negotiated version.
    // TODO: if self and peer services show witness, set feature true.
    return distributor_.notify(id, version, source);
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
    read(heading_buffer_,
        std::bind(&channel_peer::handle_read_heading,
            shared_from_base<channel_peer>(), _1, _2));
}

void channel_peer::handle_read_heading(const code& ec, size_t) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Heading read abort [" << authority() << "]");
        stop(error::channel_stopped);
        return;
    }

    if (ec)
    {
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
    read(payload_buffer_,
        std::bind(&channel_peer::handle_read_payload,
            shared_from_base<channel_peer>(), _1, _2, head));
}

// Handle errors and post message to subscribers.
// The head object is allocated on another thread and destroyed on this one.
// This introduces cross-thread allocation/deallocation, though size is small.
void channel_peer::handle_read_payload(const code& ec,
    size_t LOG_ONLY(payload_size), const heading_ptr& head) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Payload read abort [" << authority() << "]");
        stop(error::channel_stopped);
        return;
    }

    if (ec)
    {
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
    // The message object is allocated on this thread and notify invokes
    // subscribers on the same thread. This significantly reduces deallocation
    // cost in constrast to allowing the object to destroyed on another thread.
    // If object is passed to another thread destruction cost can be very high.
    const auto code = notify(head->id(), negotiated_version(), payload_buffer_);

    if (code)
    {
        if (head->command == messages::p2p::transaction::command ||
            head->command == messages::p2p::block::command)
        {
            // error::operation_failed implies null arena, not invalid payload.
            LOGR("Invalid " << head->command << " payload from [" << authority()
                << "] with hash [" << encode_hash(bitcoin_hash(payload_buffer_)) << "] "
                << code.message());
        }
        else
        {
            LOGR("Invalid " << head->command << " payload from [" << authority()
                << "] with bytes (" << encode_base16(
                    {
                        payload_buffer_.begin(),
                        std::next(payload_buffer_.begin(),
                        std::min(payload_size, invalid_payload_dump_size))
                    })
                << "...) " << code.message());
        }

        stop(code);
        return;
    }

    // Don't retain larger than configured (time-space tradeoff).
    if (settings().minimum_buffer < payload_buffer_.capacity())
    {
        payload_buffer_.resize(settings().minimum_buffer);
        payload_buffer_.shrink_to_fit();
    }

    LOGX("Recv " << head->command << " from [" << authority() << "] ("
        << payload_size << " bytes)");

    start_inactivity();
    read_heading();
}


BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
