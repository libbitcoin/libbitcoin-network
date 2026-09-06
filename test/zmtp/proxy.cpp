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

BOOST_AUTO_TEST_SUITE(zmtp_proxy_tests)

using stream = network::zmtp::stream;
using context = network::zmtp::context;
using role = network::zmtp::role;
using peer_socket = network::asio::socket;
using system::data_chunk;
using system::data_stack;

// Test infrastructure.
// ----------------------------------------------------------------------------
// The publisher runs on the pool thread, so the subscriber peer is a plain
// socket driven synchronously from the test thread (no io_context to run).

class mock_proxy
  : public proxy
{
public:
    mock_proxy(const socket::ptr& socket) NOEXCEPT
      : proxy(socket, 0)
    {
    }

    // Call must be stranded.
    void read1(http::flat_buffer& buffer, rpc::request& request,
        count_handler&& handler) NOEXCEPT
    {
        proxy::read(buffer, request, std::move(handler));
    }

    void write1(rpc::request&& notification,
        count_handler&& handler) NOEXCEPT
    {
        proxy::write(std::move(notification), std::move(handler));
    }

    void write1(rpc::response&& response,
        count_handler&& handler) NOEXCEPT
    {
        proxy::write(std::move(response), std::move(handler));
    }
};

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

// Synchronously perform the peer (SUB) side of the ZMTP handshake.
static void peer_handshake(peer_socket& peer, uint8_t minor)
{
    auto greeting = stream::make_greeting(false, false);
    greeting.at(11) = minor;
    peer_write(peer, greeting);

    data_chunk theirs(stream::greeting_size, 0x00);
    const boost::asio::mutable_buffer in{ theirs.data(), theirs.size() };
    boost::asio::read(peer, in);
    uint8_t their_minor{};
    bool curve{};
    bool as_server{};
    BOOST_REQUIRE(stream::parse_greeting(theirs, their_minor, curve, as_server));

    peer_write(peer, ready("SUB"));
    uint8_t flags{};
    data_chunk body{};
    peer_read_frame(peer, flags, body);
    BOOST_REQUIRE(!is_zero(flags & stream::flag_command));
}

// Parse a command frame body into its name and content.
static void parse_command(const data_chunk& body, std::string& name,
    data_chunk& content)
{
    std::span<const uint8_t> rest{};
    const std::span<const uint8_t> frame{ body };
    BOOST_REQUIRE(stream::command_name(name, rest, frame));
    content.assign(rest.begin(), rest.end());
}

// Send a PING and require the echoed PONG, proving all prior frames were
// consumed by the proxy (the stream is ordered).
static void peer_ping_pong(peer_socket& peer)
{
    const auto context_bytes = system::base16_chunk("0011223344556677");
    data_chunk ping{ 0x00, 0x00 };
    ping.insert(ping.end(), context_bytes.begin(), context_bytes.end());
    peer_write(peer, command("PING", ping));

    uint8_t flags{};
    data_chunk body{};
    peer_read_frame(peer, flags, body);
    BOOST_REQUIRE(!is_zero(flags & stream::flag_command));

    std::string name{};
    data_chunk content{};
    parse_command(body, name, content);
    BOOST_REQUIRE_EQUAL(name, "PONG");
    BOOST_REQUIRE_EQUAL(content, context_bytes);
}

// Accept one publisher connection with a completed ZMTP handshake.
static void accept_publisher(const socket::ptr& sock,
    asio::acceptor& acceptor, peer_socket& peer, uint8_t minor)
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

    std::promise<code> accepted{};
    sock->accept(acceptor, [&](const code& result) NOEXCEPT
    {
        accepted.set_value(result);
    });

    peer.connect(acceptor.local_endpoint());
    peer_handshake(peer, minor);
    BOOST_REQUIRE_EQUAL(accepted.get_future().get(), error::success);
}

// A publisher socket with its proxy and a handshaken peer.
struct publisher_fixture
{
    publisher_fixture()
      : pool(1),
        params{ .maximum_request = 42,
            .context = socket::context{ std::cref(configuration) },
            .role = role::publisher },
        sock(std::make_shared<network::socket>(log, pool.service(), params)),
        prx(std::make_shared<mock_proxy>(sock)),
        strand(pool.service().get_executor()),
        acceptor(strand),
        peer(peer_context)
    {
        accept_publisher(sock, acceptor, peer, 1);
    }

    ~publisher_fixture()
    {
        prx->stop(error::channel_stopped);
        peer.close();
        pool.stop();
        BOOST_REQUIRE(pool.join());
    }

    // Arm one rpc read on the proxy strand.
    std::future<code> read(rpc::request& request)
    {
        auto got = std::make_shared<std::promise<code>>();
        auto pending = got->get_future();
        boost::asio::post(prx->strand(), [=, this, &request]() NOEXCEPT
        {
            prx->read1(buffer, request,
                [got](const code& ec, size_t) NOEXCEPT
                {
                    got->set_value(ec);
                });
        });

        return pending;
    }

    const logger log{};
    threadpool pool;
    const context configuration{};
    socket::parameters params;
    socket::ptr sock;
    std::shared_ptr<mock_proxy> prx;
    asio::strand strand;
    asio::acceptor acceptor;
    boost::asio::io_context peer_context{};
    peer_socket peer;
    http::flat_buffer buffer{};
};

// The prefix param of a subscription is a byte chunk.
static data_chunk prefix_of(const rpc::request& request)
{
    const auto& params = std::get<rpc::array_t>(*request.message.params);
    const auto& any = std::get<rpc::any_t>(params.at(0).value());
    return *any.as<const data_chunk>();
}

static bool stop_of(const rpc::request& request)
{
    const auto& params = std::get<rpc::array_t>(*request.message.params);
    return std::get<rpc::boolean_t>(params.at(1).value());
}

// Accept (handshake through the socket accept path).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(zmtp_proxy__accept__v31_peer__handshake_success,
    publisher_fixture)
{
    BOOST_REQUIRE(sock);
}

// Read (frames to rpc by role, control absorbed by the proxy).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(zmtp_proxy__read__ping__pong_queued_and_read_absorbed,
    publisher_fixture)
{
    rpc::request request{};
    auto pending = read(request);

    // The PING is absorbed (PONG queued through the proxy) and the read re-armed.
    peer_ping_pong(peer);
    BOOST_REQUIRE(pending.wait_for(milliseconds(100)) == std::future_status::timeout);

    // A subscription is delivered to the channel.
    const auto topic = system::to_chunk(std::string{ "hashblock" });
    peer_write(peer, command("SUBSCRIBE", topic));
    BOOST_REQUIRE_EQUAL(pending.get(), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "subscribe");
    BOOST_REQUIRE_EQUAL(prefix_of(request), topic);
    BOOST_REQUIRE(!stop_of(request));
}

BOOST_FIXTURE_TEST_CASE(zmtp_proxy__read__pong__read_absorbed, publisher_fixture)
{
    rpc::request request{};
    auto pending = read(request);

    // An unsolicited PONG is dropped and the read re-armed.
    peer_write(peer, command("PONG", system::base16_chunk("00112233")));
    BOOST_REQUIRE(pending.wait_for(milliseconds(100)) == std::future_status::timeout);

    const auto topic = system::to_chunk(std::string{ "rawtx" });
    peer_write(peer, command("CANCEL", topic));
    BOOST_REQUIRE_EQUAL(pending.get(), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "subscribe");
    BOOST_REQUIRE_EQUAL(prefix_of(request), topic);
    BOOST_REQUIRE(stop_of(request));
}

BOOST_FIXTURE_TEST_CASE(zmtp_proxy__read__v30_subscription__delivered,
    publisher_fixture)
{
    rpc::request request{};
    auto pending = read(request);

    // The 3.0 dialect subscribes by a single 0x01-prefixed message frame.
    const auto topic = system::to_chunk(std::string{ "sequence" });
    data_chunk body{ 0x01 };
    body.insert(body.end(), topic.begin(), topic.end());
    peer_write(peer, stream::frame_encode(body, false, false));
    BOOST_REQUIRE_EQUAL(pending.get(), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "subscribe");
    BOOST_REQUIRE_EQUAL(prefix_of(request), topic);
    BOOST_REQUIRE(!stop_of(request));
}

BOOST_FIXTURE_TEST_CASE(zmtp_proxy__read__data_message__unexpected_message,
    publisher_fixture)
{
    rpc::request request{};
    auto pending = read(request);

    // A publisher receives no data messages.
    const data_chunk payload{ 0x42, 0x43 };
    peer_write(peer, stream::frame_encode(payload, false, false));
    BOOST_REQUIRE_EQUAL(pending.get(), error::zmtp_unexpected_message);
}

BOOST_FIXTURE_TEST_CASE(zmtp_proxy__read__unknown_command__unexpected_command,
    publisher_fixture)
{
    rpc::request request{};
    auto pending = read(request);
    peer_write(peer, command("BOGUS", {}));
    BOOST_REQUIRE_EQUAL(pending.get(), error::zmtp_unexpected_command);
}

// Write (rpc to frames by role).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_CASE(zmtp_proxy__write__notification__topic_and_param_frames,
    publisher_fixture)
{
    // The write is unconditional; filtering by subscription is the protocol's.
    const auto body = system::to_shared(system::to_chunk(std::string{ "body" }));
    rpc::request notification{};
    notification.message.method = "hashblock";
    notification.message.params = rpc::array_t
    {
        rpc::any_t{ body },
        rpc::value_t{ uint32_t{ 0x01020304 } }
    };

    std::promise<code> sent{};
    prx->write1(std::move(notification), [&](const code& ec, size_t) NOEXCEPT
    {
        sent.set_value(ec);
    });

    const auto received = peer_read_message(peer);
    BOOST_REQUIRE_EQUAL(sent.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(received.size(), 3u);
    BOOST_REQUIRE_EQUAL(received.at(0), system::to_chunk(std::string{ "hashblock" }));
    BOOST_REQUIRE_EQUAL(received.at(1), *body);
    BOOST_REQUIRE_EQUAL(received.at(2), system::base16_chunk("04030201"));
}

BOOST_FIXTURE_TEST_CASE(zmtp_proxy__write__response__unserializable,
    publisher_fixture)
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

BOOST_FIXTURE_TEST_CASE(zmtp_proxy__write__double_param__unserializable,
    publisher_fixture)
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
