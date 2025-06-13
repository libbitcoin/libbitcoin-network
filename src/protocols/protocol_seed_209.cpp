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
#include <bitcoin/network/protocols/protocol_seed_209.hpp>

#include <algorithm>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_seed_209

using namespace system;
using namespace messages;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_seed_209::protocol_seed_209(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    timer_(std::make_shared<deadline>(session->log, channel->strand(),
        session->settings().channel_germination())),
    tracker<protocol_seed_209>(session->log)
{
}

// Start/Stop.
// ----------------------------------------------------------------------------

void protocol_seed_209::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_209");

    if (started())
        return;

    SUBSCRIBE_CHANNEL(address, handle_receive_address, _1, _2);
    SUBSCRIBE_CHANNEL(get_address, handle_receive_get_address, _1, _2);
    SEND(get_address{}, handle_send_get_address, _1);

    protocol::start();
}

bool protocol_seed_209::complete() const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_209");

    return /*sent_address_ &&*/ sent_get_address_ && received_address_;
}

void protocol_seed_209::stopping(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_209");

    timer_->stop();
}

void protocol_seed_209::handle_timer(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_209");

    if (stopped())
        return;

    // error::operation_canceled is set when timer stopped (caught above).
    if (ec)
    {
        stop(ec);
        return;
    }

    stop(error::channel_timeout);
}

// Inbound (store addresses).
// ----------------------------------------------------------------------------

void protocol_seed_209::handle_send_get_address(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_209");

    if (stopped(ec))
        return;

    timer_->start(BIND(handle_timer, _1));
    sent_get_address_ = true;

    if (complete())
        stop(error::success);
}

address::cptr protocol_seed_209::filter(
    const address_items& items) const NOEXCEPT
{
    const size_t cap = settings().host_pool_capacity;
    const size_t gap = cap - address_count();

    // Take at least the gap or what we can get.
    const size_t minimum = std::min(gap, items.size());

    // Take up to the cap but no more.
    const size_t maximum = std::min(cap, items.size());

    // Returns zero if minimum > maximum.
    const size_t select = pseudo_random::next(minimum, maximum);
    if (is_zero(select))
        return to_shared<address>();

    // Copy, shuffle, reduce, and filter to the target amount.
    const auto message = system::to_shared<address>(items);
    auto& addresses = const_cast<address_items&>(message->addresses);
    pseudo_random::shuffle(addresses);
    addresses.resize(select);
    std::erase_if(addresses, [&](const auto& address) NOEXCEPT
    {
        return settings().excluded(address);
    });

    return message;
}

// Allow and handle any number of address messages when seeding.
bool protocol_seed_209::handle_receive_address(const code& ec,
    const address::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_209");

    if (stopped(ec))
        return false;

    const auto start_size = message->addresses.size();
    if (is_one(start_size) && (message->addresses.front() == outbound()))
    {
        ////LOGP("Dropping redundant address from seed [" << authority() << "]");
        return true;
    }

    const auto filtered = filter(message->addresses);
    const auto end_size = filtered->addresses.size();

    save(filtered, BIND(handle_save_addresses, _1, _2, end_size, start_size));

    return true;
}

void protocol_seed_209::handle_save_addresses(const code& ec,
    size_t LOG_ONLY(accepted), size_t end_size, size_t start_size) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_209");

    if (stopped())
        return;

    // The seed sent addresses but the set was filtered to zero.
    const auto emptied = (ec == error::address_not_found &&
        is_zero(end_size) && !is_zero(start_size));

    // Save error does not stop the channel.
    if (ec && !emptied)
        stop(ec);

    LOGN("Accepted (" << start_size << ">" << end_size << ">" << accepted
        << ") addresses from seed [" << authority() << "].");

    // Multiple address messages are allowed, but do not delay session.
    // Ignore a singleton message, conventional to send self upon connect.
    received_address_ = !is_one(start_size);

    if (complete())
        stop(error::success);
}

// Outbound (fetch and send addresses).
// ----------------------------------------------------------------------------

// Only send 0..1 address in response to each get_address when seeding.
bool protocol_seed_209::handle_receive_get_address(const code& ec,
    const get_address::cptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_209");

    if (stopped(ec))
        return false;

    // Advertise self if configured for inbound and with self address(es).
    if (settings().advertise_enabled())
    {
        SEND(selfs(), handle_send_address, _1);
        return true;
    }

    // handle_send_address has been bypassed, so completion here.
    handle_send_address(error::success);
    return true;
}

void protocol_seed_209::handle_send_address(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_209");

    if (stopped(ec))
        return;

    // Multiple get_address messages are allowed, but do not delay stop.
    // Dedicated seed nodes may never request addresses, so don't want on them
    // to stop. If peer sends get_address before address, it will process.
    ////sent_address_ = true;

    if (complete())
        stop(error::success);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
