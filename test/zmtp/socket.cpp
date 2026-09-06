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
#include <future>
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(zmtp_socket_tests)

using stream = network::zmtp::stream;
using context = network::zmtp::context;
using role = network::zmtp::role;
using peer_socket = network::asio::socket;
using system::data_chunk;
using system::data_stack;

// Test infrastructure.
// ----------------------------------------------------------------------------
// The server socket runs on the pool thread, so the peer is a plain socket
// driven synchronously from the test thread (no io_context to run). The
// socket reads and writes rpc messages by its role, so these cases exercise
// the mapping of each role directly on the socket (the proxy adds control
// absorption, see the proxy tests).

// Build a command frame with the given name and trailing content.
static data_chunk command(const std::string& name, const data_chunk& content)
{
    data_chunk body{};
    const auto size = system::possible_narrow_cast<uint8_t>(name.size());
    body.push_back(size);
    body.insert(body.end(), name.begin(), name.end());
    body.insert(body.end(), content.begin(), content.end());
    return stream::frame_encode(body, true, false);
}

// Build a READY command advertising the given socket type.
static data_chunk ready(const std::string& type)
{
    const std::string key{ "Socket-Type" };
    const auto key_size = system::possible_narrow_cast<uint8_t>(key.size());
    const auto type_size = system::possible_narrow_cast<uint32_t>(type.size());
    const auto value_size = system::to_big_endian(type_size);

    data_chunk meta{};
    meta.push_back(key_size);
    meta.insert(meta.end(), key.begin(), key.end());
    meta.insert(meta.end(), value_size.begin(), value_size.end());
    meta.insert(meta.end(), type.begin(), type.end());
    return command("READY", meta);
}

// Synchronously write the whole buffer to the peer socket.
static void peer_write(peer_socket& peer, const data_chunk& data)
{
    const boost::asio::const_buffer out{ data.data(), data.size() };
    boost::asio::write(peer, out);
}

// Synchronously read one short frame (flags and body) from the peer socket.
static void peer_read_frame(peer_socket& peer, uint8_t& flags,
    data_chunk& body)
{
    system::data_array<2> head{};
    const boost::asio::mutable_buffer in{ head.data(), head.size() };
    boost::asio::read(peer, in);
    flags = head.front();
    BOOST_REQUIRE(is_zero(flags & stream::flag_long));

    body.assign(head.back(), 0x00);
    if (body.empty())
        return;

    const boost::asio::mutable_buffer rest{ body.data(), body.size() };
    boost::asio::read(peer, rest);
}

// Synchronously read one whole (multipart) message from the peer socket.
static data_stack peer_read_message(peer_socket& peer)
{
    data_stack parts{};
    auto more = true;
    while (more)
    {
        uint8_t flags{};
        data_chunk body{};
        peer_read_frame(peer, flags, body);
        parts.push_back(body);
        more = !is_zero(flags & stream::flag_more);
    }

    return parts;
}

// Synchronously perform the peer side of the ZMTP handshake as the given
// socket type, returning the name of the first command received (READY or
// ERROR).
static std::string peer_handshake(peer_socket& peer, const std::string& type)
{
    peer_write(peer, stream::make_greeting(false, false));

    data_chunk theirs(stream::greeting_size, 0x00);
    const boost::asio::mutable_buffer in{ theirs.data(), theirs.size() };
    boost::asio::read(peer, in);
    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BOOST_REQUIRE(stream::parse_greeting(theirs, minor, curve, as_server));

    peer_write(peer, ready(type));
    uint8_t flags{};
    data_chunk body{};
    peer_read_frame(peer, flags, body);
    BOOST_REQUIRE(!is_zero(flags & stream::flag_command));

    std::string name{};
    std::span<const uint8_t> rest{};
    const std::span<const uint8_t> frame{ body };
    BOOST_REQUIRE(stream::command_name(name, rest, frame));
    return name;
}

// A server socket in the given role and a peer of the given socket type.
struct role_fixture
{
    role_fixture(role value, const std::string& type,
        size_t maximum=4096)
      : pool(1),
        params{ .maximum_request = maximum,
            .context = socket::context{ std::cref(configuration) },
            .role = value },
        sock(std::make_shared<network::socket>(log, pool.service(), params)),
        strand(pool.service().get_executor()),
        acceptor(strand),
        peer(peer_context)
    {
        boost_code ec{};
        acceptor.open(asio::tcp::v4(), ec);
        BOOST_REQUIRE(!ec);
        acceptor.set_option(asio::reuse_address(true), ec);
        BOOST_REQUIRE(!ec);
        acceptor.bind({ boost::asio::ip::address_v4::loopback(), 0 }, ec);
        BOOST_REQUIRE(!ec);
        acceptor.listen(1, ec);
        BOOST_REQUIRE(!ec);

        std::promise<code> promised{};
        sock->accept(acceptor, [&](const code& result) NOEXCEPT
        {
            promised.set_value(result);
        });

        peer.connect(acceptor.local_endpoint());
        first = peer_handshake(peer, type);
        accepted = promised.get_future().get();
    }

    ~role_fixture()
    {
        sock->stop();
        peer.close();
        pool.stop();
        BOOST_REQUIRE(pool.join());
    }

    // Read one rpc request from the socket (handler posted to socket strand).
    code read(rpc::request& request)
    {
        std::promise<code> got{};
        sock->rpc_read(buffer, request, [&](const code& ec, size_t) NOEXCEPT
        {
            got.set_value(ec);
        });

        return got.get_future().get();
    }

    code notify(rpc::request&& notification)
    {
        std::promise<code> sent{};
        sock->rpc_notify(std::move(notification),
            [&](const code& ec, size_t) NOEXCEPT
            {
                sent.set_value(ec);
            });

        return sent.get_future().get();
    }

    code respond(rpc::response&& response)
    {
        std::promise<code> sent{};
        sock->rpc_write(std::move(response),
            [&](const code& ec, size_t) NOEXCEPT
            {
                sent.set_value(ec);
            });

        return sent.get_future().get();
    }

    const logger log{};
    threadpool pool;
    const context configuration{};
    socket::parameters params;
    socket::ptr sock;
    asio::strand strand;
    asio::acceptor acceptor;
    boost::asio::io_context peer_context{};
    peer_socket peer;
    http::flat_buffer buffer{};
    std::string first{};
    code accepted{};
};

struct puller_fixture
  : role_fixture
{
    puller_fixture() : role_fixture(role::puller, "PUSH") {}
};

struct replier_fixture
  : role_fixture
{
    replier_fixture() : role_fixture(role::replier, "REQ") {}
};

struct router_fixture
  : role_fixture
{
    router_fixture() : role_fixture(role::router, "DEALER") {}
};

struct incompatible_fixture
  : role_fixture
{
    incompatible_fixture() : role_fixture(role::puller, "SUB") {}
};

struct bounded_fixture
  : role_fixture
{
    bounded_fixture() : role_fixture(role::puller, "PUSH", 16) {}
};

// Positional params are byte chunks.
static data_chunk param_of(const rpc::request& request, size_t index)
{
    const auto& params = std::get<rpc::array_t>(*request.message.params);
    const auto& any = std::get<rpc::any_t>(params.at(index).value());
    return *any.as<const data_chunk>();
}

static size_t params_of(const rpc::request& request)
{
    return std::get<rpc::array_t>(*request.message.params).size();
}

static data_chunk chunk(const std::string& text)
{
    return system::to_chunk(text);
}

// Compatibility (the role admits only compatible peer socket types).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(zmtp_socket__accept__incompatible_peer__error,
    incompatible_fixture)
{
    // The NULL handshake writes READY before validating the peer READY.
    BOOST_REQUIRE_EQUAL(first, "READY");
    BOOST_REQUIRE_NE(accepted, error::success);

    uint8_t flags{};
    data_chunk body{};
    peer_read_frame(peer, flags, body);
    BOOST_REQUIRE(!is_zero(flags & stream::flag_command));

    std::string name{};
    std::span<const uint8_t> rest{};
    const std::span<const uint8_t> frame{ body };
    BOOST_REQUIRE(stream::command_name(name, rest, frame));
    BOOST_REQUIRE_EQUAL(name, "ERROR");
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__accept__puller_push_peer__ready,
    puller_fixture)
{
    BOOST_REQUIRE_EQUAL(first, "READY");
    BOOST_REQUIRE_EQUAL(accepted, error::success);
}

// Control (read in every role as ping/pong requests).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__ping__ttl_and_context,
    puller_fixture)
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
    const auto& params = std::get<rpc::array_t>(*request.message.params);
    BOOST_REQUIRE_EQUAL(std::get<uint16_t>(params.at(0).value()), 0x0102u);
    BOOST_REQUIRE_EQUAL(param_of(request, 1), context_bytes);
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__notify__pong__pong_command,
    puller_fixture)
{
    const auto context_bytes = system::base16_chunk("00112233");
    rpc::request pong{};
    pong.message.method = "pong";
    pong.message.params = rpc::array_t
    {
        rpc::any_t{ system::to_shared(data_chunk{ context_bytes }) }
    };

    BOOST_REQUIRE_EQUAL(notify(std::move(pong)), error::success);

    uint8_t flags{};
    data_chunk body{};
    peer_read_frame(peer, flags, body);
    BOOST_REQUIRE(!is_zero(flags & stream::flag_command));
    const auto expected = stream::make_pong(context_bytes);
    BOOST_REQUIRE_EQUAL(body, data_chunk(std::next(expected.begin(), 2), expected.end()));
}

// Puller (reads notifications, writes nothing).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__puller_message__method_and_params,
    puller_fixture)
{
    const data_stack parts{ chunk("method"), chunk("one"), chunk("two") };
    peer_write(peer, stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "method");
    BOOST_REQUIRE(!request.message.id);
    BOOST_REQUIRE_EQUAL(params_of(request), 2u);
    BOOST_REQUIRE_EQUAL(param_of(request, 0), chunk("one"));
    BOOST_REQUIRE_EQUAL(param_of(request, 1), chunk("two"));
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__puller_empty_method__unexpected_message,
    puller_fixture)
{
    const data_stack parts{ data_chunk{}, chunk("one") };
    peer_write(peer, stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_unexpected_message);
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__puller_subscribe__unexpected_command,
    puller_fixture)
{
    // Subscriptions are read only by a publisher.
    peer_write(peer, command("SUBSCRIBE", chunk("topic")));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_unexpected_command);
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__excessive_parts__excessive_parts,
    puller_fixture)
{
    data_stack parts(add1(stream::maximum_parts), chunk("part"));
    peer_write(peer, stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_excessive_parts);
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__oversized_frame__oversized_payload,
    bounded_fixture)
{
    // The frame length exceeds the socket maximum before it is read.
    const data_stack parts{ chunk("method"), data_chunk(32, 0x42) };
    peer_write(peer, stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::oversized_payload);
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__two_messages_one_read__residue_carried,
    puller_fixture)
{
    // Both messages arrive in one read, the second is residue for the next.
    auto both = stream::frame_message({ chunk("first"), chunk("one") });
    const auto second = stream::frame_message({ chunk("second") });
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

BOOST_FIXTURE_TEST_CASE(zmtp_socket__notify__puller__unserializable,
    puller_fixture)
{
    rpc::request notification{};
    notification.message.method = "topic";
    BOOST_REQUIRE_EQUAL(notify(std::move(notification)),
        error::zmtp_unserializable);
}

// Replier (reads delimited requests, writes delimited responses).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__replier_request__delimited,
    replier_fixture)
{
    const data_stack parts{ data_chunk{}, chunk("method"), chunk("param") };
    peer_write(peer, stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE(request.message.id);
    BOOST_REQUIRE(std::holds_alternative<rpc::null_t>(*request.message.id));
    BOOST_REQUIRE_EQUAL(request.message.method, "method");
    BOOST_REQUIRE_EQUAL(params_of(request), 1u);
    BOOST_REQUIRE_EQUAL(param_of(request, 0), chunk("param"));
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__replier_undelimited__unexpected_message,
    replier_fixture)
{
    const data_stack parts{ chunk("method"), chunk("param") };
    peer_write(peer, stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_unexpected_message);
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__respond__replier_result__delimited_value,
    replier_fixture)
{
    rpc::response response{};
    response.message.result = rpc::value_t{ uint16_t{ 0x0102 } };
    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(received.size(), 2u);
    BOOST_REQUIRE(received.at(0).empty());
    BOOST_REQUIRE_EQUAL(received.at(1), system::base16_chunk("0201"));
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__respond__replier_error__code_and_message,
    replier_fixture)
{
    rpc::response response{};
    response.message.error = rpc::result_t{ .code = -32601, .message = "nope" };
    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(received.size(), 3u);
    BOOST_REQUIRE(received.at(0).empty());
    BOOST_REQUIRE_EQUAL(received.at(1),
        system::to_chunk(system::to_little_endian(int64_t{ -32601 })));
    BOOST_REQUIRE_EQUAL(received.at(2), chunk("nope"));
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__notify__replier__unserializable,
    replier_fixture)
{
    rpc::request notification{};
    notification.message.method = "topic";
    BOOST_REQUIRE_EQUAL(notify(std::move(notification)),
        error::zmtp_unserializable);
}

// Router (the rpc id is the peer identity).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__router_dealer_request__identified,
    router_fixture)
{
    const data_stack parts{ chunk("peer1"), chunk("method"), chunk("param") };
    peer_write(peer, stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE(request.message.id);
    BOOST_REQUIRE_EQUAL(std::get<rpc::string_t>(*request.message.id), "peer1");
    BOOST_REQUIRE_EQUAL(request.message.method, "method");
    BOOST_REQUIRE_EQUAL(params_of(request), 1u);
    BOOST_REQUIRE_EQUAL(param_of(request, 0), chunk("param"));
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__router_req_request__delimiter_skipped,
    router_fixture)
{
    const data_stack parts{ chunk("peer2"), data_chunk{}, chunk("method") };
    peer_write(peer, stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE_EQUAL(std::get<rpc::string_t>(*request.message.id), "peer2");
    BOOST_REQUIRE_EQUAL(request.message.method, "method");
    BOOST_REQUIRE_EQUAL(params_of(request), 0u);
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__read__router_no_request__unexpected_message,
    router_fixture)
{
    const data_stack parts{ chunk("peer3") };
    peer_write(peer, stream::frame_message(parts));

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::zmtp_unexpected_message);
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__respond__router_result__identity_envelope,
    router_fixture)
{
    rpc::response response{};
    response.message.id = rpc::string_t{ "peer1" };
    response.message.result = rpc::value_t{ rpc::string_t{ "result" } };
    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(received.size(), 3u);
    BOOST_REQUIRE_EQUAL(received.at(0), chunk("peer1"));
    BOOST_REQUIRE(received.at(1).empty());
    BOOST_REQUIRE_EQUAL(received.at(2), chunk("result"));
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__notify__router_identified__envelope_and_message,
    router_fixture)
{
    rpc::request notification{};
    notification.message.id = rpc::string_t{ "peer1" };
    notification.message.method = "topic";
    notification.message.params = rpc::array_t{ rpc::value_t{ true } };
    BOOST_REQUIRE_EQUAL(notify(std::move(notification)), error::success);

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(received.size(), 4u);
    BOOST_REQUIRE_EQUAL(received.at(0), chunk("peer1"));
    BOOST_REQUIRE(received.at(1).empty());
    BOOST_REQUIRE_EQUAL(received.at(2), chunk("topic"));
    BOOST_REQUIRE_EQUAL(received.at(3), data_chunk{ 0x01 });
}

BOOST_FIXTURE_TEST_CASE(zmtp_socket__notify__router_unidentified__unserializable,
    router_fixture)
{
    rpc::request notification{};
    notification.message.method = "topic";
    BOOST_REQUIRE_EQUAL(notify(std::move(notification)),
        error::zmtp_unserializable);
}

BOOST_AUTO_TEST_SUITE_END()
