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

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_seed_31402
static const std::string protocol_name = "seed";

using namespace bc;
using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

protocol_seed_31402::protocol_seed_31402(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    sent_address_(false),
    sent_get_address_(false),
    received_address_(false),
    timer_(std::make_shared<deadline>(session.log(), channel->strand(),
        session.settings().channel_germination())),
    tracker<protocol_seed_31402>(session.log())
{
}

const std::string& protocol_seed_31402::name() const NOEXCEPT
{
    return protocol_name;
}

// Start/Stop.
// ----------------------------------------------------------------------------

void protocol_seed_31402::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (started())
        return;

    SUBSCRIBE2(address, handle_receive_address, _1, _2);
    SUBSCRIBE2(address, handle_receive_get_address, _1, _2);
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

// Outbound [send_get_address => receive_address (save_addresses)].
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

void protocol_seed_31402::handle_receive_address(const code& ec,
    const address::ptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (stopped(ec))
        return;

    saves(message->addresses, BIND1(handle_save_addresses, _1));
}

void protocol_seed_31402::handle_save_addresses(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (stopped())
        return;

    // Save error does not stop the channel.
    if (ec)
        stop(ec);

    // Multiple address messages are allowed, but do not delay stop.
    received_address_ = true;

    if (complete())
        stop(error::success);
}

// Inbound [receive_get_address -> send_address (load_addresses)].
// ----------------------------------------------------------------------------

void protocol_seed_31402::handle_receive_get_address(const code& ec,
    const get_address::ptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_seed_31402");

    if (stopped(ec))
        return;

    // Only send self address when seeding.
    if (is_zero(settings().self.port()))
    {
        // handle_send_address has been bypassed, so invoke here.
        handle_send_address(error::success);
        return;
    }

    static const auto item = settings().self.to_address_item();
    SEND1(address{ { item } }, handle_send_address, _1);
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

} // namespace network
} // namespace libbitcoin
