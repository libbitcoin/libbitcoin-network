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
using tcp_socket = network::asio::socket;
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
    void read1(stream::frame& out, count_handler&& handler) NOEXCEPT
    {
        proxy::read(out, std::move(handler));
    }

    void write1(const system::chunk_cptr& packet,
        count_handler&& handler) NOEXCEPT
    {
        proxy::write(packet, std::move(handler));
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
static void peer_write(tcp_socket& peer, const data_chunk& data)
{
    const boost::asio::const_buffer out{ data.data(), data.size() };
    boost::asio::write(peer, out);
}

// Synchronously read one short frame (flags and body) from the peer socket.
static void peer_read_frame(tcp_socket& peer, uint8_t& flags,
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
static data_stack peer_read_message(tcp_socket& peer)
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
static void peer_handshake(tcp_socket& peer, uint8_t minor)
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
static void peer_ping_pong(tcp_socket& peer)
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
    asio::acceptor& acceptor, tcp_socket& peer, uint8_t minor)
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

// Accept (handshake through the socket accept path).
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(zmtp_proxy__accept__v31_peer__handshake_success)
{
    const logger log{};
    threadpool pool(1);
    const context configuration{};
    socket::parameters params{ .maximum_request = 42, .context = socket::context{ std::cref(configuration) } };
    const auto sock = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    const auto prx = std::make_shared<mock_proxy>(sock);
    asio::strand strand(pool.service().get_executor());
    asio::acceptor acceptor(strand);
    boost::asio::io_context peer_context{};
    tcp_socket peer{ peer_context };
    accept_publisher(sock, acceptor, peer, 1);

    prx->stop(error::channel_stopped);
    peer.close();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

// Interception.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(zmtp_proxy__read__ping__pong_queued_and_read_absorbed)
{
    const logger log{};
    threadpool pool(1);
    const context configuration{};
    socket::parameters params{ .maximum_request = 42, .context = socket::context{ std::cref(configuration) } };
    const auto sock = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    const auto prx = std::make_shared<mock_proxy>(sock);
    asio::strand strand(pool.service().get_executor());
    asio::acceptor acceptor(strand);
    boost::asio::io_context peer_context{};
    tcp_socket peer{ peer_context };
    accept_publisher(sock, acceptor, peer, 1);

    stream::frame frame{};
    std::promise<code> got{};
    boost::asio::post(prx->strand(), [&]() NOEXCEPT
    {
        prx->read1(frame, [&](const code& ec, size_t) NOEXCEPT { got.set_value(ec); });
    });

    // The PING is absorbed (PONG queued through the proxy) and the read re-armed.
    peer_ping_pong(peer);
    auto pending = got.get_future();
    BOOST_REQUIRE(pending.wait_for(milliseconds(100)) == std::future_status::timeout);

    // A data frame is delivered to the channel.
    const data_chunk payload{ 0x42, 0x43 };
    peer_write(peer, stream::frame_encode(payload, false, false));
    BOOST_REQUIRE_EQUAL(pending.get(), error::success);
    BOOST_REQUIRE(!frame.command());
    BOOST_REQUIRE_EQUAL(frame.body, payload);

    prx->stop(error::channel_stopped);
    peer.close();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__read__subscribe_command__delivered_to_channel)
{
    const logger log{};
    threadpool pool(1);
    const context configuration{};
    socket::parameters params{ .maximum_request = 42, .context = socket::context{ std::cref(configuration) } };
    const auto sock = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    const auto prx = std::make_shared<mock_proxy>(sock);
    asio::strand strand(pool.service().get_executor());
    asio::acceptor acceptor(strand);
    boost::asio::io_context peer_context{};
    tcp_socket peer{ peer_context };
    accept_publisher(sock, acceptor, peer, 1);

    // A subscription is the protocol's concern, so it reaches the channel.
    stream::frame frame{};
    std::promise<code> got{};
    boost::asio::post(prx->strand(), [&]() NOEXCEPT
    {
        prx->read1(frame, [&](const code& ec, size_t) NOEXCEPT { got.set_value(ec); });
    });

    const auto topic = system::to_chunk(std::string{ "hashblock" });
    peer_write(peer, command("SUBSCRIBE", topic));
    BOOST_REQUIRE_EQUAL(got.get_future().get(), error::success);
    BOOST_REQUIRE(frame.command());

    std::string name{};
    data_chunk content{};
    parse_command(frame.body, name, content);
    BOOST_REQUIRE_EQUAL(name, "SUBSCRIBE");
    BOOST_REQUIRE_EQUAL(content, topic);

    prx->stop(error::channel_stopped);
    peer.close();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_CASE(zmtp_proxy__write__framed_packet__three_frames_received)
{
    const logger log{};
    threadpool pool(1);
    const context configuration{};
    socket::parameters params{ .maximum_request = 42, .context = socket::context{ std::cref(configuration) } };
    const auto sock = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    const auto prx = std::make_shared<mock_proxy>(sock);
    asio::strand strand(pool.service().get_executor());
    asio::acceptor acceptor(strand);
    boost::asio::io_context peer_context{};
    tcp_socket peer{ peer_context };
    accept_publisher(sock, acceptor, peer, 1);

    // The write is unconditional; filtering by subscription is the protocol's.
    const auto topic = system::to_chunk(std::string{ "hashblock" });
    const auto body = system::to_chunk(std::string{ "body" });
    const data_stack parts{ topic, body, data_chunk{ 0x00, 0x00, 0x00, 0x00 } };
    const auto packet = system::to_shared(stream::frame_message(parts));
    std::promise<std::pair<code, size_t>> sent{};
    prx->write1(packet, [&](const code& ec, size_t size) NOEXCEPT { sent.set_value({ ec, size }); });
    const auto received = peer_read_message(peer);
    const auto result = sent.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::success);
    BOOST_REQUIRE_EQUAL(result.second, packet->size());
    BOOST_REQUIRE_EQUAL(received.size(), 3u);
    BOOST_REQUIRE_EQUAL(received.at(0), topic);
    BOOST_REQUIRE_EQUAL(received.at(1), body);

    prx->stop(error::channel_stopped);
    peer.close();
    pool.stop();
    BOOST_REQUIRE(pool.join());
}

BOOST_AUTO_TEST_SUITE_END()
