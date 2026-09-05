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
#include <optional>
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(zmtp_stream_tests)

using stream = network::zmtp::stream;
using context = network::zmtp::context;
using tcp_socket = network::asio::socket;
using system::data_chunk;
using system::data_stack;
using system::base16_chunk;

// Test infrastructure.
// ----------------------------------------------------------------------------

// Establish a connected local socket pair on the given service.
static void connect_pair(boost::asio::io_context& service,
    tcp_socket& server, tcp_socket& client)
{
    boost::asio::ip::tcp::acceptor acceptor{ service,
        { boost::asio::ip::address_v4::loopback(), 0 } };

    bool accepted{};
    bool connected{};
    acceptor.async_accept(server, [&](const boost_code& ec)
    {
        BOOST_REQUIRE(!ec);
        accepted = true;
    });
    client.async_connect(acceptor.local_endpoint(), [&](const boost_code& ec)
    {
        BOOST_REQUIRE(!ec);
        connected = true;
    });

    service.run();
    service.restart();
    BOOST_REQUIRE(accepted && connected);
}

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

// A minimal raw ZMTP SUB peer over a plain tcp socket, driven on the service.
class raw_peer
{
public:
    using ec_handler = std::function<void(const boost_code&)>;
    using frame_handler = std::function<void(const boost_code&, uint8_t,
        const data_chunk&)>;

    raw_peer(tcp_socket& socket, uint8_t minor) NOEXCEPT
      : socket_(socket), minor_(minor)
    {
    }

    void handshake(boost_code& out)
    {
        auto greeting = stream::make_greeting(false, false);
        greeting.at(11) = minor_;
        write(greeting);

        read_exactly(stream::greeting_size, [this, &out](const boost_code& ec)
        {
            if (ec) { out = ec; return; }
            write(ready("SUB"));
            read_frame([&out](const boost_code& code, uint8_t,
                const data_chunk&) { out = code; });
        });
    }

    void subscribe(const data_chunk& topic)
    {
        if (is_zero(minor_))
        {
            data_chunk body{ 0x01 };
            body.insert(body.end(), topic.begin(), topic.end());
            write(stream::frame_encode(body, false, false));
            return;
        }

        write(command("SUBSCRIBE", topic));
    }

    void ping(const data_chunk& context_bytes)
    {
        data_chunk body{ 0x00, 0x00 };
        body.insert(body.end(), context_bytes.begin(), context_bytes.end());
        write(command("PING", body));
    }

    void read_message(std::vector<data_chunk>& parts, boost_code& out)
    {
        read_frame([this, &parts, &out](const boost_code& ec, uint8_t flags,
            const data_chunk& body)
        {
            if (ec) { out = ec; return; }
            parts.push_back(body);
            out = ec;
            const auto more = !is_zero(flags & stream::flag_more);
            if (more) read_message(parts, out);
        });
    }

    void read_frame(const frame_handler& handler)
    {
        read_exactly(1, [this, handler](const boost_code& ec)
        {
            if (ec) { handler(ec, uint8_t{}, {}); return; }
            const auto flags = scratch_.front();
            const auto longer = !is_zero(flags & stream::flag_long);
            const auto length_size = longer ? sizeof(uint64_t) : one;
            read_exactly(length_size,
                [this, handler, flags, longer](const boost_code& code)
            {
                if (code) { handler(code, flags, {}); return; }
                const size_t length = longer ?
                    system::from_big_endian<uint64_t>(
                        system::unsafe_array_cast<uint8_t, 8>(scratch_.data())) :
                    scratch_.front();
                read_exactly(length,
                    [this, handler, flags](const boost_code& done)
                {
                    handler(done, flags, scratch_);
                });
            });
        });
    }

    void read_exactly(size_t size, const ec_handler& handler)
    {
        scratch_.assign(size, 0x00);
        const boost::asio::mutable_buffer in{ scratch_.data(),
            scratch_.size() };
        boost::asio::async_read(socket_, in,
            [handler](const boost_code& ec, size_t) { handler(ec); });
    }

    void write(const data_chunk& data)
    {
        const auto buffer = system::to_shared(data);
        const boost::asio::const_buffer out{ buffer->data(),
            buffer->size() };
        boost::asio::async_write(socket_, out,
            [buffer](const boost_code&, size_t) {});
    }

private:
    tcp_socket& socket_;
    const uint8_t minor_;
    data_chunk scratch_{};
};

// A CURVE client over the raw peer, driven by a client cipher.
class curve_peer
  : public raw_peer
{
public:
    using cipher = network::zmtp::cipher;

    curve_peer(tcp_socket& socket, cipher& client) NOEXCEPT
      : raw_peer(socket, 1), client_(client)
    {
    }

    void handshake(boost_code& out)
    {
        write(stream::make_greeting(false, true));
        read_exactly(stream::greeting_size, [this, &out](const boost_code& ec)
        {
            if (ec) { out = ec; return; }
            data_chunk hello{};
            if (!client_.hello(hello)) { out = failure(); return; }
            write(stream::frame_encode(hello, true, false));
            read_frame([this, &out](const boost_code& code, uint8_t,
                const data_chunk& welcome)
            {
                if (code) { out = code; return; }
                data_chunk initiate{};
                const auto metadata = stream::make_property("Socket-Type",
                    "SUB");
                if (!client_.initiate(initiate, welcome, metadata))
                {
                    out = failure();
                    return;
                }

                write(stream::frame_encode(initiate, true, false));
                read_frame([this, &out](const boost_code& done, uint8_t,
                    const data_chunk& ready)
                {
                    if (done) { out = done; return; }
                    data_chunk metadata{};
                    std::string type{};
                    const auto ok = client_.complete(metadata, ready) &&
                        stream::ready_socket_type(type, metadata) &&
                        type == "PUB";
                    out = ok ? done : failure();
                });
            });
        });
    }

    // Box and send one frame (command flag as a payload flag).
    void send(uint8_t payload, const data_chunk& body)
    {
        data_chunk message{};
        BOOST_REQUIRE(client_.encode(message, payload, body));
        write(stream::frame_encode(message, true, false));
    }

    void subscribe(const data_chunk& topic)
    {
        data_chunk body{};
        const std::string name{ "SUBSCRIBE" };
        body.push_back(system::possible_narrow_cast<uint8_t>(name.size()));
        body.insert(body.end(), name.begin(), name.end());
        body.insert(body.end(), topic.begin(), topic.end());
        send(cipher::payload_command, body);
    }

    // Read and unbox one whole multipart message.
    void read_message(std::vector<data_chunk>& parts, boost_code& out)
    {
        read_frame([this, &parts, &out](const boost_code& ec, uint8_t flags,
            const data_chunk& message)
        {
            if (ec) { out = ec; return; }
            uint8_t payload{};
            data_chunk body{};
            if (is_zero(flags & stream::flag_command) ||
                !client_.decode(payload, body, message))
            {
                out = failure();
                return;
            }

            parts.push_back(body);
            out = ec;
            if (!is_zero(payload & cipher::payload_more))
                read_message(parts, out);
        });
    }

private:
    static boost_code failure()
    {
        return boost::asio::error::no_protocol_option;
    }

    cipher& client_;
};

// Codec (static).
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(zmtp_stream__make_greeting__round_trip__minor_one)
{
    const auto greeting = stream::make_greeting(false, false);
    BOOST_REQUIRE_EQUAL(greeting.size(), stream::greeting_size);
    BOOST_REQUIRE_EQUAL(greeting.front(), 0xffu);
    BOOST_REQUIRE_EQUAL(greeting.at(9), 0x7fu);
    BOOST_REQUIRE_EQUAL(greeting.at(10), 3u);
    BOOST_REQUIRE_EQUAL(greeting.at(11), 1u);

    uint8_t minor{ 0xff };
    bool curve{};
    bool as_server{};
    BOOST_REQUIRE(stream::parse_greeting(greeting, minor, curve, as_server));
    BOOST_REQUIRE_EQUAL(minor, 1u);
}

BOOST_AUTO_TEST_CASE(zmtp_stream__parse_greeting__bad_signature__false)
{
    auto greeting = stream::make_greeting(false, false);
    greeting.front() = 0x00;

    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BOOST_REQUIRE(!stream::parse_greeting(greeting, minor, curve, as_server));
}

BOOST_AUTO_TEST_CASE(zmtp_stream__parse_greeting__low_major__false)
{
    auto greeting = stream::make_greeting(false, false);
    greeting.at(10) = 2u;

    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BOOST_REQUIRE(!stream::parse_greeting(greeting, minor, curve, as_server));
}

BOOST_AUTO_TEST_CASE(zmtp_stream__parse_greeting__wrong_mechanism__false)
{
    auto greeting = stream::make_greeting(false, false);
    greeting.at(12) = 'C';

    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BOOST_REQUIRE(!stream::parse_greeting(greeting, minor, curve, as_server));
}

BOOST_AUTO_TEST_CASE(zmtp_stream__frame_encode__short__expected)
{
    const data_chunk body(3, 0x11);
    const auto frame = stream::frame_encode(body, false, false);
    BOOST_REQUIRE_EQUAL(frame.at(0), 0x00u);
    BOOST_REQUIRE_EQUAL(frame.at(1), 3u);
    BOOST_REQUIRE_EQUAL(frame.size(), 5u);
}

BOOST_AUTO_TEST_CASE(zmtp_stream__frame_encode__long_command_more__expected)
{
    const data_chunk body(300, 0x22);
    const auto frame = stream::frame_encode(body, true, true);
    const uint8_t expected_flags = stream::flag_long | stream::flag_command | stream::flag_more;
    BOOST_REQUIRE_EQUAL(frame.at(0), expected_flags);
    BOOST_REQUIRE_EQUAL(frame.at(7), 300u / 256u);
    BOOST_REQUIRE_EQUAL(frame.at(8), 300u % 256u);
    BOOST_REQUIRE_EQUAL(frame.size(), 300u + 9u);
}

BOOST_AUTO_TEST_CASE(zmtp_stream__frame_message__three_parts__more_on_all_but_last)
{
    const data_stack parts
    {
        system::to_chunk(std::string{ "topic" }),
        system::to_chunk(std::string{ "body" }),
        data_chunk{ 0x00, 0x00, 0x00, 0x00 }
    };

    const auto packet = stream::frame_message(parts);

    // topic: flags(MORE) len(5) 5 bytes; body: flags(MORE) len(4) 4 bytes; seq: flags(0) len(4) 4 bytes.
    BOOST_REQUIRE_EQUAL(packet.size(), 7u + 6u + 6u);
    BOOST_REQUIRE_EQUAL(packet.at(0), stream::flag_more);
    BOOST_REQUIRE_EQUAL(packet.at(1), 5u);
    BOOST_REQUIRE_EQUAL(packet.at(7), stream::flag_more);
    BOOST_REQUIRE_EQUAL(packet.at(8), 4u);
    BOOST_REQUIRE_EQUAL(packet.at(13), 0x00u);
    BOOST_REQUIRE_EQUAL(packet.at(14), 4u);
}

BOOST_AUTO_TEST_CASE(zmtp_stream__make_ready_pub__socket_type__pub)
{
    const auto frame = stream::make_ready_pub();
    BOOST_REQUIRE(!is_zero(frame.at(0) & stream::flag_command));

    const std::span<const uint8_t> body{ std::next(frame.data(), 2u), frame.size() - 2u };
    std::string name{};
    std::span<const uint8_t> meta{};
    BOOST_REQUIRE(stream::command_name(name, meta, body));
    BOOST_REQUIRE_EQUAL(name, "READY");

    std::string type{};
    BOOST_REQUIRE(stream::ready_socket_type(type, meta));
    BOOST_REQUIRE_EQUAL(type, "PUB");
}

BOOST_AUTO_TEST_CASE(zmtp_stream__make_pong__echoes_truncated_context)
{
    const data_chunk context_bytes(20, 0xab);
    const auto frame = stream::make_pong(context_bytes);

    const std::span<const uint8_t> body{ std::next(frame.data(), 2u), frame.size() - 2u };
    std::string name{};
    std::span<const uint8_t> echo{};
    BOOST_REQUIRE(stream::command_name(name, echo, body));
    BOOST_REQUIRE_EQUAL(name, "PONG");
    BOOST_REQUIRE_EQUAL(echo.size(), stream::maximum_ping_context);
}

// Handshake and framing (loopback against a raw SUB peer).
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(zmtp_stream__async_write__v31_peer__three_frames_received)
{
    boost::asio::io_context service{};
    tcp_socket server{ service };
    tcp_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    std::optional<stream> publisher{};
    publisher.emplace(std::move(server), configuration);
    raw_peer peer{ client, 1 };

    boost_code shook{ boost::asio::error::would_block };
    boost_code peer_shook{ boost::asio::error::would_block };
    publisher->async_handshake(true, [&](const boost_code& ec) { shook = ec; });
    peer.handshake(peer_shook);
    service.run();
    service.restart();
    BOOST_REQUIRE(!shook);
    BOOST_REQUIRE(!peer_shook);

    const auto topic = system::to_chunk(std::string{ "hashblock" });
    const auto body = system::to_chunk(std::string{ "body" });
    const data_stack parts{ topic, body, data_chunk{ 0x00, 0x00, 0x00, 0x00 } };
    const auto packet = system::to_shared(stream::frame_message(parts));

    boost_code sent{ boost::asio::error::would_block };
    size_t sent_size{};
    const boost::asio::const_buffer out{ packet->data(), packet->size() };
    publisher->async_write(out, [&](const boost_code& ec, size_t size)
    {
        sent = ec;
        sent_size = size;
    });

    std::vector<data_chunk> received{};
    boost_code got{ boost::asio::error::would_block };
    peer.read_message(received, got);
    service.run();
    service.restart();

    BOOST_REQUIRE(!sent);
    BOOST_REQUIRE_EQUAL(sent_size, packet->size());
    BOOST_REQUIRE(!got);
    BOOST_REQUIRE_EQUAL(received.size(), 3u);
    BOOST_REQUIRE_EQUAL(received.at(0), topic);
    BOOST_REQUIRE_EQUAL(received.at(1), body);

    publisher->next_layer().close();
    client.close();
}

BOOST_AUTO_TEST_CASE(zmtp_stream__handshake__v30_peer__minor_zero)
{
    boost::asio::io_context service{};
    tcp_socket server{ service };
    tcp_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    std::optional<stream> publisher{};
    publisher.emplace(std::move(server), configuration);
    raw_peer peer{ client, 0 };

    boost_code shook{ boost::asio::error::would_block };
    boost_code peer_shook{ boost::asio::error::would_block };
    publisher->async_handshake(true, [&](const boost_code& ec) { shook = ec; });
    peer.handshake(peer_shook);
    service.run();
    service.restart();
    BOOST_REQUIRE(!shook);
    BOOST_REQUIRE(!peer_shook);

    publisher->next_layer().close();
    client.close();
}

BOOST_AUTO_TEST_CASE(zmtp_stream__async_read_frame__subscribe_command__surfaced)
{
    boost::asio::io_context service{};
    tcp_socket server{ service };
    tcp_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    std::optional<stream> publisher{};
    publisher.emplace(std::move(server), configuration);
    raw_peer peer{ client, 1 };

    boost_code shook{ boost::asio::error::would_block };
    boost_code peer_shook{ boost::asio::error::would_block };
    publisher->async_handshake(true, [&](const boost_code& ec) { shook = ec; });
    peer.handshake(peer_shook);
    service.run();
    service.restart();
    BOOST_REQUIRE(!shook);
    BOOST_REQUIRE(!peer_shook);

    const auto topic = system::to_chunk(std::string{ "hashblock" });
    peer.subscribe(topic);

    boost_code got{ boost::asio::error::would_block };
    stream::frame frame{};
    publisher->async_read_frame(frame, [&](const boost_code& ec, size_t) { got = ec; });
    service.run();
    service.restart();
    BOOST_REQUIRE(!got);
    BOOST_REQUIRE(frame.command());
    BOOST_REQUIRE(!frame.more());

    std::string name{};
    std::span<const uint8_t> content{};
    const std::span<const uint8_t> body{ frame.body };
    BOOST_REQUIRE(stream::command_name(name, content, body));
    BOOST_REQUIRE_EQUAL(name, "SUBSCRIBE");
    const data_chunk subscribed{ content.begin(), content.end() };
    BOOST_REQUIRE_EQUAL(subscribed, topic);

    publisher->next_layer().close();
    client.close();
}

BOOST_AUTO_TEST_CASE(zmtp_stream__async_read_frame__ping_command__pong_built_from_context)
{
    boost::asio::io_context service{};
    tcp_socket server{ service };
    tcp_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    std::optional<stream> publisher{};
    publisher.emplace(std::move(server), configuration);
    raw_peer peer{ client, 1 };

    boost_code shook{ boost::asio::error::would_block };
    boost_code peer_shook{ boost::asio::error::would_block };
    publisher->async_handshake(true, [&](const boost_code& ec) { shook = ec; });
    peer.handshake(peer_shook);
    service.run();
    service.restart();
    BOOST_REQUIRE(!shook);
    BOOST_REQUIRE(!peer_shook);

    const auto context_bytes = system::base16_chunk("0011223344556677");
    peer.ping(context_bytes);

    boost_code got{ boost::asio::error::would_block };
    stream::frame frame{};
    publisher->async_read_frame(frame, [&](const boost_code& ec, size_t) { got = ec; });
    service.run();
    service.restart();
    BOOST_REQUIRE(!got);
    BOOST_REQUIRE(frame.command());

    // The tier above intercepts PING: parse it and build the PONG to queue.
    std::string name{};
    std::span<const uint8_t> content{};
    const std::span<const uint8_t> body{ frame.body };
    BOOST_REQUIRE(stream::command_name(name, content, body));
    BOOST_REQUIRE_EQUAL(name, "PING");
    const auto ping_context = content.subspan(sizeof(uint16_t));
    const auto pong = stream::make_pong(ping_context);

    std::string pong_name{};
    std::span<const uint8_t> echo{};
    const std::span<const uint8_t> pong_body{ std::next(pong.data(), 2u), pong.size() - 2u };
    BOOST_REQUIRE(stream::command_name(pong_name, echo, pong_body));
    BOOST_REQUIRE_EQUAL(pong_name, "PONG");
    const data_chunk echoed{ echo.begin(), echo.end() };
    BOOST_REQUIRE_EQUAL(echoed, context_bytes);

    publisher->next_layer().close();
    client.close();
}

// CURVE mechanism (server).
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(zmtp_stream__make_greeting__curve_server__mechanism_and_as_server)
{
    const auto greeting = stream::make_greeting(true, true);
    BOOST_REQUIRE_EQUAL(greeting.size(), stream::greeting_size);
    BOOST_REQUIRE_EQUAL(std::string(std::next(greeting.begin(), 12), std::next(greeting.begin(), 17)), "CURVE");
    BOOST_REQUIRE_EQUAL(greeting.at(32), 0x01u);

    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BOOST_REQUIRE(stream::parse_greeting(greeting, minor, curve, as_server));
    BOOST_REQUIRE_EQUAL(minor, 1u);
    BOOST_REQUIRE(curve);
    BOOST_REQUIRE(as_server);

    const auto client = stream::make_greeting(false, true);
    BOOST_REQUIRE_EQUAL(client.at(32), 0x00u);
    BOOST_REQUIRE(stream::parse_greeting(client, minor, curve, as_server));
    BOOST_REQUIRE(curve);
    BOOST_REQUIRE(!as_server);
}

BOOST_AUTO_TEST_CASE(zmtp_stream__frame_decode__two_frames__expected)
{
    const data_stack parts{ { 0x01, 0x02 }, data_chunk(300, 0x42) };
    const auto packet = stream::frame_message(parts);
    std::span<const uint8_t> buffer{ packet };

    uint8_t flags{};
    std::span<const uint8_t> body{};
    BOOST_REQUIRE(stream::frame_decode(flags, body, buffer));
    BOOST_REQUIRE_EQUAL(flags, stream::flag_more);
    BOOST_REQUIRE_EQUAL(body.size(), 2u);
    BOOST_REQUIRE(stream::frame_decode(flags, body, buffer));
    BOOST_REQUIRE_EQUAL(flags, stream::flag_long);
    BOOST_REQUIRE_EQUAL(body.size(), 300u);
    BOOST_REQUIRE(buffer.empty());
    BOOST_REQUIRE(!stream::frame_decode(flags, body, buffer));
}

BOOST_AUTO_TEST_CASE(zmtp_stream__curve_handshake__valid_client__completes)
{
    boost::asio::io_context service{};
    tcp_socket server_socket{ service };
    tcp_socket client_socket{ service };
    connect_pair(service, server_socket, client_socket);

    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    network::zmtp::cipher::key client_secret{};
    network::zmtp::cipher::key client_public{};
    system::x25519::generate(server_secret, server_public);
    system::x25519::generate(client_secret, client_public);
    const context curve{ system::to_chunk(server_secret) };
    BOOST_REQUIRE(curve.curve());
    BOOST_REQUIRE_EQUAL(curve.public_key(), server_public);

    stream server{ std::move(server_socket), curve };
    network::zmtp::cipher client{ client_secret, client_public, server_public };
    curve_peer peer{ client_socket, client };

    boost_code server_result{ boost::asio::error::would_block };
    boost_code peer_result{ boost::asio::error::would_block };
    server.async_handshake(true, [&](const boost_code& ec) { server_result = ec; });
    peer.handshake(peer_result);
    service.run();
    BOOST_REQUIRE_MESSAGE(!server_result, server_result.message());
    BOOST_REQUIRE_MESSAGE(!peer_result, peer_result.message());
}

BOOST_AUTO_TEST_CASE(zmtp_stream__curve_handshake__null_client__error)
{
    boost::asio::io_context service{};
    tcp_socket server_socket{ service };
    tcp_socket client_socket{ service };
    connect_pair(service, server_socket, client_socket);

    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    system::x25519::generate(server_secret, server_public);
    const context curve{ system::to_chunk(server_secret) };

    stream server{ std::move(server_socket), curve };
    raw_peer peer{ client_socket, 1 };

    boost_code server_result{ boost::asio::error::would_block };
    boost_code peer_result{ boost::asio::error::would_block };
    server.async_handshake(true, [&](const boost_code& ec) { server_result = ec; });
    peer.handshake(peer_result);
    service.run();
    BOOST_REQUIRE_EQUAL(server_result, boost_code(boost::asio::error::no_protocol_option));
}

BOOST_AUTO_TEST_CASE(zmtp_stream__context__wrong_size_secret__null_mechanism)
{
    const context short_secret{ data_chunk(31, 0x01) };
    BOOST_REQUIRE(!short_secret.curve());

    const context none{};
    BOOST_REQUIRE(!none.curve());
}

BOOST_AUTO_TEST_CASE(zmtp_stream__curve__subscribe_and_publish__unboxed_both_ways)
{
    boost::asio::io_context service{};
    tcp_socket server_socket{ service };
    tcp_socket client_socket{ service };
    connect_pair(service, server_socket, client_socket);

    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    network::zmtp::cipher::key client_secret{};
    network::zmtp::cipher::key client_public{};
    system::x25519::generate(server_secret, server_public);
    system::x25519::generate(client_secret, client_public);
    const context curve{ system::to_chunk(server_secret) };

    stream server{ std::move(server_socket), curve };
    network::zmtp::cipher client{ client_secret, client_public, server_public };
    curve_peer peer{ client_socket, client };

    boost_code server_result{ boost::asio::error::would_block };
    boost_code peer_result{ boost::asio::error::would_block };
    server.async_handshake(true, [&](const boost_code& ec) { server_result = ec; });
    peer.handshake(peer_result);
    service.run();
    service.restart();
    BOOST_REQUIRE(!server_result && !peer_result);

    // The boxed SUBSCRIBE surfaces as a plain command frame.
    stream::frame frame{};
    boost_code read_result{ boost::asio::error::would_block };
    server.async_read_frame(frame, [&](const boost_code& ec, size_t) { read_result = ec; });
    const data_chunk topic{ 0x68, 0x61, 0x73, 0x68 };
    peer.subscribe(topic);
    service.run();
    service.restart();
    BOOST_REQUIRE_MESSAGE(!read_result, read_result.message());
    BOOST_REQUIRE(frame.command());
    BOOST_REQUIRE(!frame.more());
    BOOST_REQUIRE_EQUAL(frame.body, base16_chunk("0953554253435249424568617368"));

    // The framed message is boxed per frame and unboxed by the peer.
    const data_stack parts{ topic, data_chunk(300, 0x42), { 0x01, 0x00, 0x00, 0x00 } };
    const auto packet = stream::frame_message(parts);
    boost_code write_result{ boost::asio::error::would_block };
    boost_code message_result{ boost::asio::error::would_block };
    std::vector<data_chunk> received{};
    server.async_write({ packet.data(), packet.size() }, [&](const boost_code& ec, size_t) { write_result = ec; });
    peer.read_message(received, message_result);
    service.run();
    BOOST_REQUIRE_MESSAGE(!write_result, write_result.message());
    BOOST_REQUIRE_MESSAGE(!message_result, message_result.message());
    BOOST_REQUIRE_EQUAL(received.size(), 3u);
    BOOST_REQUIRE_EQUAL(received.at(0), parts.at(0));
    BOOST_REQUIRE_EQUAL(received.at(1), parts.at(1));
    BOOST_REQUIRE_EQUAL(received.at(2), parts.at(2));
}

BOOST_AUTO_TEST_SUITE_END()
