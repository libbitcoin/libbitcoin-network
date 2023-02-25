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
#include <bitcoin/network/protocols/protocol_seed_31402.hpp>

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

#define CLASS protocol_seed_31402

using namespace system;
using namespace messages;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

protocol_seed_31402::protocol_seed_31402(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    timer_(std::make_shared<deadline>(session.log(), channel->strand(),
        session.settings().channel_germination())),
    tracker<protocol_seed_31402>(session.log())
{
}

// Start/Stop.
// ----------------------------------------------------------------------------

void protocol_seed_31402::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (started())
        return;

    SUBSCRIBE2(address, handle_receive_address, _1, _2);
    SUBSCRIBE2(get_address, handle_receive_get_address, _1, _2);
    SEND1(get_address{}, handle_send_get_address, _1);

    protocol::start();
}

bool protocol_seed_31402::complete() const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    return sent_address_ && sent_get_address_ && received_address_;
}

void protocol_seed_31402::stopping(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    timer_->stop();
}

void protocol_seed_31402::handle_timer(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

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

void protocol_seed_31402::handle_send_get_address(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (stopped(ec))
        return;

    timer_->start(BIND1(handle_timer, _1));
    sent_get_address_ = true;

    if (complete())
        stop(error::success);
}

address::cptr protocol_seed_31402::filter(
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

    // CLang doesn't like emplacement with default constructors, so use new.
    BC_PUSH_WARNING(NO_NEW_OR_DELETE)
    const auto message = std::shared_ptr<address>(new address{ items });
    BC_POP_WARNING()

    // Shuffle, reduce, and filter to the target amount.
    pseudo_random::shuffle(message->addresses);
    message->addresses.resize(select);
    std::erase_if(message->addresses, [&](const auto& address) NOEXCEPT
    {
        return settings().excluded(address);
    });

    return message;
}

// Allow and handle any number of address messages when seeding.
void protocol_seed_31402::handle_receive_address(const code& ec,
    const address::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (stopped(ec))
        return;

    const auto start = message->addresses.size();
    if (is_one(start) && (message->addresses.front() == outbound()))
    {
        ////LOGP("Dropping redundant address from seed [" << authority() << "]");
        return;
    }

    const auto filtered = filter(message->addresses);
    const auto end = filtered->addresses.size();

    save(filtered,
        BIND4(handle_save_addresses, _1, _2, end, start));
}

void protocol_seed_31402::handle_save_addresses(const code& ec,
    size_t LOG_ONLY(accepted), size_t LOG_ONLY(filtered),
    size_t start) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (stopped())
        return;

    // Save error does not stop the channel.
    if (ec)
        stop(ec);

    LOGN("Accepted (" << start << ">" << filtered << ">" << accepted << ") "
        "addresses from seed [" << authority() << "].");

    // Multiple address messages are allowed, but do not delay session.
    // Ignore a singleton message, conventional to send self upon connect.
    received_address_ = !is_one(start);

    if (complete())
        stop(error::success);
}

// Outbound (fetch and send addresses).
// ----------------------------------------------------------------------------

address_item protocol_seed_31402::self() const NOEXCEPT
{
    return settings().self.to_address_item(unix_time(),
        settings().services_maximum);
}

// Only send 0..1 address in response to each get_address when seeding.
void protocol_seed_31402::handle_receive_get_address(const code& ec,
    const get_address::cptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (stopped(ec))
        return;

    // Advertise self if configured for inbound and valid self address.
    if (settings().advertise_enabled())
    {
        SEND1(address{ { self() } }, handle_send_address, _1);
        return;
    }

    // handle_send_address has been bypassed, so completion here.
    handle_send_address(error::success);
}

void protocol_seed_31402::handle_send_address(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (stopped(ec))
        return;

    // Multiple get_address messages are allowed, but do not delay stop.
    sent_address_ = true;

    if (complete())
        stop(error::success);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
