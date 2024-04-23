/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/protocols/protocol_ping_60001.hpp>

#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/protocols/protocol_ping_31402.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_ping_60001

using namespace system;
using namespace messages;
using namespace std::placeholders;

constexpr uint64_t received = zero;
constexpr auto minimum_nonce = add1(received);

protocol_ping_60001::protocol_ping_60001(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol_ping_31402(session, channel),
    nonce_(received),
    tracker<protocol_ping_60001>(session->log)
{
}

void protocol_ping_60001::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_60001");

    if (started())
        return;

    SUBSCRIBE_CHANNEL(pong, handle_receive_pong, _1, _2);
    ////SUBSCRIBE_CHANNEL(ping, handle_receive_ping, _1, _2);
    ////send_ping();

    protocol_ping_31402::start();
}

////void protocol_ping_31402::stopping(const code&) NOEXCEPT
////{
////    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");
////
////    timer_->stop();
////}

// Outgoing (send_ping [on timer] => receive_pong [with timeout]).
// ----------------------------------------------------------------------------

void protocol_ping_60001::send_ping() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_60001");

    if (stopped())
        return;

    // The ping/pong nonce is arbitrary and distinct from the channel nonce.
    nonce_ = pseudo_random::next<uint64_t>(add1(minimum_nonce), bc::max_int64);
    SEND(ping{ nonce_ }, handle_send, _1);
}

////void protocol_ping_31402::handle_send_ping(const code& ec) NOEXCEPT
////{
////    BC_ASSERT_MSG(stranded(), "protocol_ping_31402");
////
////    if (stopped(ec))
////        return;
////
////    timer_->start(BIND(handle_timer, _1));
////    protocol::handle_send(ec);
////}

bool protocol_ping_60001::handle_receive_pong(const code& ec,
    const pong::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_60001");

    if (stopped(ec))
        return false;

    // Both nonce incorrect and already received are protocol violations.
    if (message->nonce != nonce_)
    {
        LOGR("Incorrect pong nonce from [" << authority() << "]");
        stop(error::protocol_violation);
        return false;
    }

    // Correct pong nonce, set sentinel.
    nonce_ = received;
    return true;
}

void protocol_ping_60001::handle_timer(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_60001");

    if (stopped())
        return;

    // error::operation_canceled implies stopped, so this is something else.
    if (ec)
    {
        // TODO: log code.
        stop(ec);
        return;
    }

    // No error code on timeout, so check for nonce receipt.
    if (nonce_ != received)
    {
        // TODO: log ping timeout.
        stop(ec);
        return;
    }

    // Correct nonce received before timeout, time to send another ping.
    send_ping();
}

// Incoming (receive_ping => send_pong).
// ----------------------------------------------------------------------------

bool protocol_ping_60001::handle_receive_ping(const code& ec,
    const ping::cptr& message) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_60001");

    if (stopped(ec))
        return false;

    SEND(pong{ message->nonce }, handle_send_pong, _1);
    return true;
}

void protocol_ping_60001::handle_send_pong(const code&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_ping_60001");
}

} // namespace network
} // namespace libbitcoin
