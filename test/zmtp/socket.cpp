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

// The socket reads and writes rpc messages by its role, so these cases
// exercise the mapping of each role directly on the socket (the proxy adds
// control absorption, see the proxy tests).

using system::data_chunk;
using system::data_stack;

// Compatibility (the role admits only compatible peer socket types).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_socket_incompatible_tests, incompatible_fixture)

BOOST_AUTO_TEST_CASE(zmtp_socket__accept__incompatible_peer__error)
{
    // The NULL handshake writes READY before validating the peer READY.
    BOOST_REQUIRE_EQUAL(first, "READY");
    BOOST_REQUIRE_NE(accepted, error::success);

    uint8_t flags{};
    data_chunk body{};
    peer_read_frame(peer, flags, body);
    BOOST_REQUIRE(!is_zero(flags & zmtp_stream::flag_command));

    std::string name{};
    data_chunk content{};
    parse_command(body, name, content);
    BOOST_REQUIRE_EQUAL(name, "ERROR");
}

BOOST_AUTO_TEST_SUITE_END()

// Puller (reads notifications, writes nothing), and control (every role).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_socket_puller_tests, puller_fixture)

BOOST_AUTO_TEST_CASE(zmtp_socket__accept__puller_push_peer__ready)
{
    BOOST_REQUIRE_EQUAL(first, "READY");
    BOOST_REQUIRE_EQUAL(accepted, error::success);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__read__ping__ttl_and_context)
{
    // TTL is big-endian on the wire.
    const auto context_bytes = system::base16_chunk("0011223344556677");
    data_chunk ping{ 0x01, 0x02 };
    ping.insert(ping.end(), context_bytes.begin(), context_bytes.end());
    peer_write(peer, command("PING", ping));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "ping");
    BOOST_REQUIRE_EQUAL(params_of(request), 2u);

    const auto& parameters = std::get<rpc::array_t>(*request.message.params);
    BOOST_REQUIRE_EQUAL(std::get<uint16_t>(parameters.at(0).value()), 0x0102u);
    BOOST_REQUIRE_EQUAL(param_of(request, 1), context_bytes);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__notify__pong__pong_command)
{
    const auto context_bytes = system::base16_chunk("00112233");
    rpc::request pong{};
    pong.message.method = "pong";
    pong.message.params = rpc::array_t{ rpc::any_t{ system::to_shared(data_chunk{ context_bytes }) } };
    BOOST_REQUIRE_EQUAL(notify(std::move(pong)), error::success);

    uint8_t flags{};
    data_chunk body{};
    peer_read_frame(peer, flags, body);
    BOOST_REQUIRE(!is_zero(flags & zmtp_stream::flag_command));

    std::string name{};
    data_chunk content{};
    parse_command(body, name, content);
    BOOST_REQUIRE_EQUAL(name, "PONG");
    BOOST_REQUIRE_EQUAL(content, context_bytes);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__read__puller_message__method_and_params)
{
    const data_stack parts{ chunk("method"), chunk("one"), chunk("two") };
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "method");
    BOOST_REQUIRE(!request.message.id);
    BOOST_REQUIRE_EQUAL(params_of(request), 2u);
    BOOST_REQUIRE_EQUAL(param_of(request, 0), chunk("one"));
    BOOST_REQUIRE_EQUAL(param_of(request, 1), chunk("two"));
}

BOOST_AUTO_TEST_CASE(zmtp_socket__read__puller_empty_method__unexpected_message)
{
    const data_stack parts{ data_chunk{}, chunk("one") };
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_unexpected_message);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__read__puller_subscribe__unexpected_command)
{
    // Subscriptions are read only by a publisher.
    peer_write(peer, command("SUBSCRIBE", chunk("topic")));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_unexpected_command);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__read__excessive_parts__excessive_parts)
{
    data_stack parts(add1(zmtp_stream::maximum_parts), chunk("part"));
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_excessive_parts);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__read__two_messages_one_write__read_in_order)
{
    // Both messages arrive together and are read one per read.
    auto both = zmtp_stream::frame_message({ chunk("first"), chunk("one") });
    const auto second = zmtp_stream::frame_message({ chunk("second") });
    both.insert(both.end(), second.begin(), second.end());
    peer_write(peer, both);

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "first");
    BOOST_REQUIRE_EQUAL(params_of(request), 1u);

    rpc::request next{};
    BOOST_REQUIRE_EQUAL(read(next), error::success);
    BOOST_REQUIRE_EQUAL(next.message.method, "second");
    BOOST_REQUIRE_EQUAL(params_of(next), 0u);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__notify__puller__unserializable)
{
    rpc::request notification{};
    notification.message.method = "topic";
    BOOST_REQUIRE_EQUAL(notify(std::move(notification)), error::zmtp_unserializable);
}

BOOST_AUTO_TEST_SUITE_END()

// The message is bounded by the socket maximum.
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_socket_bounded_tests, bounded_fixture)

BOOST_AUTO_TEST_CASE(zmtp_socket__read__oversized_frame__oversized_payload)
{
    // The frame length exceeds the socket maximum before it is read.
    const data_stack parts{ chunk("method"), data_chunk(32, 0x42) };
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::oversized_payload);
}

BOOST_AUTO_TEST_SUITE_END()

// Replier (reads delimited requests, writes delimited responses).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_socket_replier_tests, replier_fixture)

BOOST_AUTO_TEST_CASE(zmtp_socket__read__replier_request__delimited)
{
    const data_stack parts{ data_chunk{}, chunk("method"), chunk("param") };
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE(request.message.id);
    BOOST_REQUIRE(std::holds_alternative<rpc::null_t>(*request.message.id));
    BOOST_REQUIRE_EQUAL(request.message.method, "method");
    BOOST_REQUIRE_EQUAL(params_of(request), 1u);
    BOOST_REQUIRE_EQUAL(param_of(request, 0), chunk("param"));
}

BOOST_AUTO_TEST_CASE(zmtp_socket__read__replier_undelimited__unexpected_message)
{
    const data_stack parts{ chunk("method"), chunk("param") };
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_unexpected_message);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__respond__replier_result__delimited_value)
{
    rpc::response response{};
    response.message.result = rpc::value_t{ uint16_t{ 0x0102 } };
    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(received.size(), 2u);
    BOOST_REQUIRE(received.at(0).empty());
    BOOST_REQUIRE_EQUAL(received.at(1), system::base16_chunk("0201"));
}

BOOST_AUTO_TEST_CASE(zmtp_socket__respond__replier_error__code_and_message)
{
    rpc::response response{};
    response.message.error = rpc::result_t{ .code = -32601, .message = "nope" };
    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(received.size(), 3u);
    BOOST_REQUIRE(received.at(0).empty());
    BOOST_REQUIRE_EQUAL(received.at(1), system::to_chunk(system::to_little_endian(int64_t{ -32601 })));
    BOOST_REQUIRE_EQUAL(received.at(2), chunk("nope"));
}

BOOST_AUTO_TEST_CASE(zmtp_socket__notify__replier__unserializable)
{
    rpc::request notification{};
    notification.message.method = "topic";
    BOOST_REQUIRE_EQUAL(notify(std::move(notification)), error::zmtp_unserializable);
}

BOOST_AUTO_TEST_SUITE_END()

// Router (the rpc id is the handshake identity, the envelope a delimiter).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_socket_router_tests, router_fixture)

BOOST_AUTO_TEST_CASE(zmtp_socket__read__router_dealer_request__undelimited)
{
    const data_stack parts{ chunk("method"), chunk("param") };
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE(request.message.id);
    BOOST_REQUIRE(std::holds_alternative<rpc::null_t>(*request.message.id));
    BOOST_REQUIRE_EQUAL(request.message.method, "method");
    BOOST_REQUIRE_EQUAL(params_of(request), 1u);
    BOOST_REQUIRE_EQUAL(param_of(request, 0), chunk("param"));
}

BOOST_AUTO_TEST_CASE(zmtp_socket__read__router_req_request__delimiter_skipped)
{
    const data_stack parts{ data_chunk{}, chunk("method") };
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "method");
    BOOST_REQUIRE_EQUAL(params_of(request), 0u);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__read__router_delimiter_only__unexpected_message)
{
    const data_stack parts{ data_chunk{} };
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_unexpected_message);
}

BOOST_AUTO_TEST_CASE(zmtp_socket__respond__router_result__delimited_value)
{
    rpc::response response{};
    response.message.result = rpc::value_t{ rpc::string_t{ "result" } };
    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(received.size(), 2u);
    BOOST_REQUIRE(received.at(0).empty());
    BOOST_REQUIRE_EQUAL(received.at(1), chunk("result"));
}

BOOST_AUTO_TEST_CASE(zmtp_socket__notify__router__delimited_message)
{
    rpc::request notification{};
    notification.message.method = "topic";
    notification.message.params = rpc::array_t{ rpc::value_t{ true } };
    BOOST_REQUIRE_EQUAL(notify(std::move(notification)), error::success);

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(received.size(), 3u);
    BOOST_REQUIRE(received.at(0).empty());
    BOOST_REQUIRE_EQUAL(received.at(1), chunk("topic"));
    BOOST_REQUIRE_EQUAL(received.at(2), data_chunk{ 0x01 });
}

BOOST_AUTO_TEST_SUITE_END()

// The peer identity is a handshake metadata property.
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_socket_identified_tests, identified_router_fixture)

BOOST_AUTO_TEST_CASE(zmtp_socket__read__router_identified_peer__identity_id)
{
    const data_stack parts{ data_chunk{}, chunk("method") };
    peer_write(peer, zmtp_stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE(request.message.id);
    BOOST_REQUIRE_EQUAL(std::get<rpc::string_t>(*request.message.id), "peer1");
}

BOOST_AUTO_TEST_SUITE_END()
