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
#include "../test.hpp"
#include "zmtp_setup_fixture.hpp"

// The proxy absorbs control (ping/pong) on the rpc read path, so the channel
// is keepalive-blind. Every other message is delivered as an rpc request.

using system::data_chunk;
using system::data_stack;

// Accept, read (frames to rpc, control absorbed) and write (rpc to frames).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_proxy_tests, publisher_setup_fixture)

BOOST_AUTO_TEST_CASE(zmtp_proxy__accept__v31_peer__handshake_success)
{
    BOOST_REQUIRE(sock);
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__read__ping__pong_queued_and_read_absorbed)
{
    rpc::request request{};
    auto pending = read(request);

    // The PING is absorbed (PONG queued by the proxy) and the read re-armed.
    peer_ping_pong(peer);
    BOOST_REQUIRE(pending.wait_for(milliseconds(100)) == std::future_status::timeout);

    // A subscription is delivered to the channel.
    const auto topic = chunk("hashblock");
    peer_write(peer, command("SUBSCRIBE", topic));
    BOOST_REQUIRE_EQUAL(pending.get(), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "subscribe");
    BOOST_REQUIRE_EQUAL(prefix_of(request), topic);
    BOOST_REQUIRE(!stop_of(request));
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__read__pong__read_absorbed)
{
    rpc::request request{};
    auto pending = read(request);

    // An unsolicited PONG is dropped and the read re-armed.
    peer_write(peer, command("PONG", system::base16_chunk("00112233")));
    BOOST_REQUIRE(pending.wait_for(milliseconds(100)) == std::future_status::timeout);

    const auto topic = chunk("rawtx");
    peer_write(peer, command("CANCEL", topic));
    BOOST_REQUIRE_EQUAL(pending.get(), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "subscribe");
    BOOST_REQUIRE_EQUAL(prefix_of(request), topic);
    BOOST_REQUIRE(stop_of(request));
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__read__v30_subscription__delivered)
{
    rpc::request request{};
    auto pending = read(request);

    // The 3.0 dialect subscribes by a single 0x01-prefixed message frame.
    const auto topic = chunk("sequence");
    data_chunk body{ 0x01 };
    body.insert(body.end(), topic.begin(), topic.end());
    peer_write(peer, zmtp_stream::frame_encode(body, false, false));
    BOOST_REQUIRE_EQUAL(pending.get(), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "subscribe");
    BOOST_REQUIRE_EQUAL(prefix_of(request), topic);
    BOOST_REQUIRE(!stop_of(request));
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__read__data_message__unexpected_message)
{
    rpc::request request{};
    auto pending = read(request);

    // A publisher receives no data messages.
    const data_chunk payload{ 0x42, 0x43 };
    peer_write(peer, zmtp_stream::frame_encode(payload, false, false));
    BOOST_REQUIRE_EQUAL(pending.get(), error::zmtp_unexpected_message);
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__read__unknown_command__unexpected_command)
{
    rpc::request request{};
    auto pending = read(request);
    peer_write(peer, command("BOGUS", {}));
    BOOST_REQUIRE_EQUAL(pending.get(), error::zmtp_unexpected_command);
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__write__notification__topic_and_param_frames)
{
    // The write is unconditional, filtering by subscription is the protocol's.
    const auto body = system::to_shared(chunk("body"));
    rpc::request notification{};
    notification.message.method = "hashblock";
    notification.message.params = rpc::array_t{ rpc::any_t{ body }, rpc::value_t{ uint32_t{ 0x01020304 } } };

    std::promise<code> sent{};
    prx->write1(std::move(notification), [&](const code& ec, size_t) NOEXCEPT
    {
        sent.set_value(ec);
    });

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(sent.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(received.size(), 3u);
    BOOST_REQUIRE_EQUAL(received.at(0), chunk("hashblock"));
    BOOST_REQUIRE_EQUAL(received.at(1), *body);
    BOOST_REQUIRE_EQUAL(received.at(2), system::base16_chunk("04030201"));
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__write__response__unserializable)
{
    // A publisher has no reply path.
    rpc::response response{};
    response.message.result = rpc::value_t{ rpc::string_t{ "result" } };

    std::promise<code> sent{};
    prx->write1(std::move(response), [&](const code& ec, size_t) NOEXCEPT
    {
        sent.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(sent.get_future().get(), error::zmtp_unserializable);
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__write__double_param__unserializable)
{
    rpc::request notification{};
    notification.message.method = "hashblock";
    notification.message.params = rpc::array_t{ rpc::value_t{ 42.0 } };

    std::promise<code> sent{};
    prx->write1(std::move(notification), [&](const code& ec, size_t) NOEXCEPT
    {
        sent.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(sent.get_future().get(), error::zmtp_unserializable);
}

BOOST_AUTO_TEST_SUITE_END()
