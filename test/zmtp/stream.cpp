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

BC_AUTO_TEST_SUITE(zmtp_stream_tests)

using stream = network::zmtp::stream;
using context = network::zmtp::context;
using role = network::zmtp::role;
using peer_socket = network::io::socket;
using system::data_chunk;
using system::data_stack;
using system::base16_chunk;

// Test infrastructure.
// ----------------------------------------------------------------------------

// Establish a connected local socket pair on the given service.
static void connect_pair(io::context& service,
    peer_socket& server, peer_socket& client)
{
    io::acceptor acceptor{ service,
        test::loopback(0) };

    bool accepted{};
    bool connected{};
    acceptor.async_accept(server, [&](const code& ec)
    {
        BC_REQUIRE(!ec);
        accepted = true;
    });
    client.async_connect(acceptor.local_endpoint(), [&](const code& ec)
    {
        BC_REQUIRE(!ec);
        connected = true;
    });

    service.run();
    service.restart();
    BC_REQUIRE(accepted && connected);
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
    using ec_handler = std::function<void(const code&)>;
    using frame_handler = std::function<void(const code&, uint8_t,
        const data_chunk&)>;

    raw_peer(peer_socket& socket, uint8_t minor) NOEXCEPT
      : socket_(socket), minor_(minor)
    {
    }

    void handshake(code& out)
    {
        auto greeting = stream::make_greeting(false, false);
        greeting.at(11) = minor_;
        write(greeting);

        read_exactly(stream::greeting_size, [this, &out](const code& ec)
        {
            if (ec) { out = ec; return; }
            write(ready("SUB"));
            read_frame([&out](const code& result, uint8_t,
                const data_chunk&) { out = result; });
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

    void read_message(std::vector<data_chunk>& parts, code& out)
    {
        read_frame([this, &parts, &out](const code& ec, uint8_t flags,
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
        read_exactly(1, [this, handler](const code& ec)
        {
            if (ec) { handler(ec, uint8_t{}, {}); return; }
            const auto flags = scratch_.front();
            const auto longer = !is_zero(flags & stream::flag_long);
            const auto length_size = longer ? sizeof(uint64_t) : one;
            read_exactly(length_size,
                [this, handler, flags](const code& result)
            {
                if (result) { handler(result, flags, {}); return; }
                const auto size = system::from_big_endian<uint64_t>(scratch_);
                const auto length = system::possible_narrow_cast<size_t>(size);
                read_exactly(length,
                    [this, handler, flags](const code& done)
                {
                    handler(done, flags, scratch_);
                });
            });
        });
    }

    void read_exactly(size_t size, const ec_handler& handler)
    {
        scratch_.assign(size, 0x00);
        const io::mutable_buffer in{ scratch_.data(),
            scratch_.size() };
        io::async_read(socket_, in,
            [handler](const code& ec, size_t) { handler(ec); });
    }

    void write(const data_chunk& data)
    {
        const auto buffer = system::to_shared(data);
        const io::const_buffer out{ buffer->data(),
            buffer->size() };
        io::async_write(socket_, out,
            [buffer](const code&, size_t) {});
    }

private:
    peer_socket& socket_;
    const uint8_t minor_;
    data_chunk scratch_{};
};

// A CURVE client over the raw peer, driven by a client cipher.
class curve_peer
  : public raw_peer
{
public:
    using cipher = network::zmtp::cipher;

    curve_peer(peer_socket& socket, cipher& client,
        const std::string& type="SUB") NOEXCEPT
      : raw_peer(socket, 1), client_(client), type_(type)
    {
    }

    void handshake(code& out)
    {
        write(stream::make_greeting(false, true));
        read_exactly(stream::greeting_size, [this, &out](const code& ec)
        {
            if (ec) { out = ec; return; }
            data_chunk hello{};
            if (!client_.hello(hello)) { out = failure(); return; }
            write(stream::frame_encode(hello, true, false));
            read_frame([this, &out](const code& result, uint8_t,
                const data_chunk& welcome)
            {
                if (result) { out = result; return; }
                data_chunk initiate{};
                const auto metadata = stream::make_property("Socket-Type",
                    type_);
                if (!client_.initiate(initiate, welcome, metadata))
                {
                    out = failure();
                    return;
                }

                write(stream::frame_encode(initiate, true, false));
                read_frame([this, &out](const code& done, uint8_t,
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
        BC_REQUIRE(client_.encode(message, payload, body));
        write(stream::frame_encode(message, false, false));
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
    void read_message(std::vector<data_chunk>& parts, code& out)
    {
        read_frame([this, &parts, &out](const code& ec, uint8_t,
            const data_chunk& message)
        {
            if (ec) { out = ec; return; }
            uint8_t payload{};
            data_chunk body{};
            if (!client_.decode(payload, body, message))
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
    static code failure()
    {
        return error::protocol_violation;
    }

    cipher& client_;
    const std::string type_;
};

static const code protocol_error{ error::protocol_violation };
static const code end_of_file{ error::peer_disconnect };

static code shake(io::context& service, stream& instance, bool as_server)
{
    code result{ error::would_block };
    instance.async_handshake(as_server, [&](const code& ec) { result = ec; });
    service.run();
    service.restart();
    return result;
}

static code read_one(io::context& service, stream& instance, stream::frame& out)
{
    code result{ error::would_block };
    instance.async_read_frame(out, 1024, [&](const code& ec, size_t) { result = ec; });
    service.run();
    service.restart();
    return result;
}

// A CURVE server stream and a client cipher with fresh long-term keys.
struct curve_setup
{
    curve_setup()
    {
        system::x25519::generate(server_secret, server_public);
        system::x25519::generate(client_secret, client_public);
        connect_pair(service, server_socket, client_socket);
    }

    io::context service{};
    peer_socket server_socket{ service };
    peer_socket client_socket{ service };
    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    network::zmtp::cipher::key client_secret{};
    network::zmtp::cipher::key client_public{};
};

// Codec (static).
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__make_greeting__round_trip__minor_one)
{
    const auto greeting = stream::make_greeting(false, false);
    BC_REQUIRE_EQUAL(greeting.size(), stream::greeting_size);
    BC_REQUIRE_EQUAL(greeting.front(), 0xffu);
    BC_REQUIRE_EQUAL(greeting.at(9), 0x7fu);
    BC_REQUIRE_EQUAL(greeting.at(10), 3u);
    BC_REQUIRE_EQUAL(greeting.at(11), 1u);

    uint8_t minor{ 0xff };
    bool curve{};
    bool as_server{};
    BC_REQUIRE(stream::parse_greeting(greeting, minor, curve, as_server));
    BC_REQUIRE_EQUAL(minor, 1u);
}

BC_AUTO_TEST_CASE(zmtp_stream__parse_greeting__bad_signature__false)
{
    auto greeting = stream::make_greeting(false, false);
    greeting.front() = 0x00;

    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BC_REQUIRE(!stream::parse_greeting(greeting, minor, curve, as_server));
}

BC_AUTO_TEST_CASE(zmtp_stream__parse_greeting__low_major__false)
{
    auto greeting = stream::make_greeting(false, false);
    greeting.at(10) = 2u;

    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BC_REQUIRE(!stream::parse_greeting(greeting, minor, curve, as_server));
}

BC_AUTO_TEST_CASE(zmtp_stream__parse_greeting__wrong_mechanism__false)
{
    auto greeting = stream::make_greeting(false, false);
    greeting.at(12) = 'C';

    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BC_REQUIRE(!stream::parse_greeting(greeting, minor, curve, as_server));
}

BC_AUTO_TEST_CASE(zmtp_stream__frame_encode__short__expected)
{
    const data_chunk body(3, 0x11);
    const auto frame = stream::frame_encode(body, false, false);
    BC_REQUIRE_EQUAL(frame.at(0), 0x00u);
    BC_REQUIRE_EQUAL(frame.at(1), 3u);
    BC_REQUIRE_EQUAL(frame.size(), 5u);
}

BC_AUTO_TEST_CASE(zmtp_stream__frame_encode__long_command_more__expected)
{
    const data_chunk body(300, 0x22);
    const auto frame = stream::frame_encode(body, true, true);
    const uint8_t expected_flags = stream::flag_long | stream::flag_command | stream::flag_more;
    BC_REQUIRE_EQUAL(frame.at(0), expected_flags);
    BC_REQUIRE_EQUAL(frame.at(7), 300u / 256u);
    BC_REQUIRE_EQUAL(frame.at(8), 300u % 256u);
    BC_REQUIRE_EQUAL(frame.size(), 300u + 9u);
}

BC_AUTO_TEST_CASE(zmtp_stream__frame_size__short_and_long__expected)
{
    BC_REQUIRE_EQUAL(stream::frame_size(0), 2u);
    BC_REQUIRE_EQUAL(stream::frame_size(255), 257u);
    BC_REQUIRE_EQUAL(stream::frame_size(256), 265u);
}

BC_AUTO_TEST_CASE(zmtp_stream__write_frame_head__long_command_more__frame_encode_head)
{
    const data_chunk body(300, 0x22);
    const auto frame = stream::frame_encode(body, true, true);
    const data_chunk expected(frame.begin(), std::next(frame.begin(), 9));

    data_chunk head(9);
    system::stream::out::fast sink{ head };
    system::write::bytes::fast writer{ sink };
    stream::write_frame_head(writer, body.size(), true, true);
    BC_REQUIRE(writer);
    BC_REQUIRE(head == expected);
}

BC_AUTO_TEST_CASE(zmtp_stream__frame_message__three_parts__more_on_all_but_last)
{
    const data_stack parts
    {
        system::to_chunk(std::string{ "topic" }),
        system::to_chunk(std::string{ "body" }),
        data_chunk{ 0x00, 0x00, 0x00, 0x00 }
    };

    const auto packet = stream::frame_message(parts);

    // topic: flags(MORE) len(5) 5 bytes; body: flags(MORE) len(4) 4 bytes; seq: flags(0) len(4) 4 bytes.
    BC_REQUIRE_EQUAL(packet.size(), 7u + 6u + 6u);
    BC_REQUIRE_EQUAL(packet.at(0), stream::flag_more);
    BC_REQUIRE_EQUAL(packet.at(1), 5u);
    BC_REQUIRE_EQUAL(packet.at(7), stream::flag_more);
    BC_REQUIRE_EQUAL(packet.at(8), 4u);
    BC_REQUIRE_EQUAL(packet.at(13), 0x00u);
    BC_REQUIRE_EQUAL(packet.at(14), 4u);
}

BC_AUTO_TEST_CASE(zmtp_stream__make_ready__publisher__socket_type_pub)
{
    const auto frame = stream::make_ready(role::publisher);
    BC_REQUIRE(!is_zero(frame.at(0) & stream::flag_command));

    const std::span<const uint8_t> body{ std::next(frame.data(), 2u), frame.size() - 2u };
    std::string name{};
    std::span<const uint8_t> meta{};
    BC_REQUIRE(stream::command_name(name, meta, body));
    BC_REQUIRE_EQUAL(name, "READY");

    std::string type{};
    BC_REQUIRE(stream::ready_socket_type(type, meta));
    BC_REQUIRE_EQUAL(type, "PUB");
}

BC_AUTO_TEST_CASE(zmtp_stream__make_pong__echoes_truncated_context)
{
    const data_chunk context_bytes(20, 0xab);
    const auto frame = stream::make_pong(context_bytes);

    const std::span<const uint8_t> body{ std::next(frame.data(), 2u), frame.size() - 2u };
    std::string name{};
    std::span<const uint8_t> echo{};
    BC_REQUIRE(stream::command_name(name, echo, body));
    BC_REQUIRE_EQUAL(name, "PONG");
    BC_REQUIRE_EQUAL(echo.size(), stream::maximum_ping_context);
}

// Handshake and framing (loopback against a raw SUB peer).
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__async_write__v31_peer__three_frames_received)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    std::optional<stream> publisher{};
    publisher.emplace(std::move(server), configuration, role::publisher);
    raw_peer peer{ client, 1 };

    code shook{ error::would_block };
    code peer_shook{ error::would_block };
    publisher->async_handshake(true, [&](const code& ec) { shook = ec; });
    peer.handshake(peer_shook);
    service.run();
    service.restart();
    BC_REQUIRE(!shook);
    BC_REQUIRE(!peer_shook);

    const auto topic = system::to_chunk(std::string{ "hashblock" });
    const auto body = system::to_chunk(std::string{ "body" });
    const data_stack parts{ topic, body, data_chunk{ 0x00, 0x00, 0x00, 0x00 } };
    const auto packet = system::to_shared(stream::frame_message(parts));

    code sent{ error::would_block };
    size_t sent_size{};
    const io::const_buffer out{ packet->data(), packet->size() };
    publisher->async_write(out, [&](const code& ec, size_t size)
    {
        sent = ec;
        sent_size = size;
    });

    std::vector<data_chunk> received{};
    code got{ error::would_block };
    peer.read_message(received, got);
    service.run();
    service.restart();

    BC_REQUIRE(!sent);
    BC_REQUIRE_EQUAL(sent_size, packet->size());
    BC_REQUIRE(!got);
    BC_REQUIRE_EQUAL(received.size(), 3u);
    BC_REQUIRE_EQUAL(received.at(0), topic);
    BC_REQUIRE_EQUAL(received.at(1), body);

    publisher->next_layer().close();
    client.close();
}

BC_AUTO_TEST_CASE(zmtp_stream__handshake__v30_peer__minor_zero)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    std::optional<stream> publisher{};
    publisher.emplace(std::move(server), configuration, role::publisher);
    raw_peer peer{ client, 0 };

    code shook{ error::would_block };
    code peer_shook{ error::would_block };
    publisher->async_handshake(true, [&](const code& ec) { shook = ec; });
    peer.handshake(peer_shook);
    service.run();
    service.restart();
    BC_REQUIRE(!shook);
    BC_REQUIRE(!peer_shook);

    publisher->next_layer().close();
    client.close();
}

BC_AUTO_TEST_CASE(zmtp_stream__async_read_frame__subscribe_command__surfaced)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    std::optional<stream> publisher{};
    publisher.emplace(std::move(server), configuration, role::publisher);
    raw_peer peer{ client, 1 };

    code shook{ error::would_block };
    code peer_shook{ error::would_block };
    publisher->async_handshake(true, [&](const code& ec) { shook = ec; });
    peer.handshake(peer_shook);
    service.run();
    service.restart();
    BC_REQUIRE(!shook);
    BC_REQUIRE(!peer_shook);

    const auto topic = system::to_chunk(std::string{ "hashblock" });
    peer.subscribe(topic);

    code got{ error::would_block };
    stream::frame frame{};
    publisher->async_read_frame(frame, 1024, [&](const code& ec, size_t) { got = ec; });
    service.run();
    service.restart();
    BC_REQUIRE(!got);
    BC_REQUIRE(frame.command());
    BC_REQUIRE(!frame.more());

    std::string name{};
    std::span<const uint8_t> content{};
    const std::span<const uint8_t> body{ frame.body };
    BC_REQUIRE(stream::command_name(name, content, body));
    BC_REQUIRE_EQUAL(name, "SUBSCRIBE");
    const data_chunk subscribed{ content.begin(), content.end() };
    BC_REQUIRE_EQUAL(subscribed, topic);

    publisher->next_layer().close();
    client.close();
}

BC_AUTO_TEST_CASE(zmtp_stream__async_read_frame__ping_command__pong_built_from_context)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    std::optional<stream> publisher{};
    publisher.emplace(std::move(server), configuration, role::publisher);
    raw_peer peer{ client, 1 };

    code shook{ error::would_block };
    code peer_shook{ error::would_block };
    publisher->async_handshake(true, [&](const code& ec) { shook = ec; });
    peer.handshake(peer_shook);
    service.run();
    service.restart();
    BC_REQUIRE(!shook);
    BC_REQUIRE(!peer_shook);

    const auto context_bytes = system::base16_chunk("0011223344556677");
    peer.ping(context_bytes);

    code got{ error::would_block };
    stream::frame frame{};
    publisher->async_read_frame(frame, 1024, [&](const code& ec, size_t) { got = ec; });
    service.run();
    service.restart();
    BC_REQUIRE(!got);
    BC_REQUIRE(frame.command());

    // The tier above intercepts PING: parse it and build the PONG to queue.
    std::string name{};
    std::span<const uint8_t> content{};
    const std::span<const uint8_t> body{ frame.body };
    BC_REQUIRE(stream::command_name(name, content, body));
    BC_REQUIRE_EQUAL(name, "PING");
    const auto ping_context = content.subspan(sizeof(uint16_t));
    const auto pong = stream::make_pong(ping_context);

    std::string pong_name{};
    std::span<const uint8_t> echo{};
    const std::span<const uint8_t> pong_body{ std::next(pong.data(), 2u), pong.size() - 2u };
    BC_REQUIRE(stream::command_name(pong_name, echo, pong_body));
    BC_REQUIRE_EQUAL(pong_name, "PONG");
    const data_chunk echoed{ echo.begin(), echo.end() };
    BC_REQUIRE_EQUAL(echoed, context_bytes);

    publisher->next_layer().close();
    client.close();
}

// CURVE mechanism (server).
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__make_greeting__curve_server__mechanism_and_as_server)
{
    const auto greeting = stream::make_greeting(true, true);
    BC_REQUIRE_EQUAL(greeting.size(), stream::greeting_size);
    BC_REQUIRE_EQUAL(std::string(std::next(greeting.begin(), 12), std::next(greeting.begin(), 17)), "CURVE");
    BC_REQUIRE_EQUAL(greeting.at(32), 0x01u);

    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BC_REQUIRE(stream::parse_greeting(greeting, minor, curve, as_server));
    BC_REQUIRE_EQUAL(minor, 1u);
    BC_REQUIRE(curve);
    BC_REQUIRE(as_server);

    const auto client = stream::make_greeting(false, true);
    BC_REQUIRE_EQUAL(client.at(32), 0x00u);
    BC_REQUIRE(stream::parse_greeting(client, minor, curve, as_server));
    BC_REQUIRE(curve);
    BC_REQUIRE(!as_server);
}

BC_AUTO_TEST_CASE(zmtp_stream__frame_decode__two_frames__expected)
{
    const data_stack parts{ { 0x01, 0x02 }, data_chunk(300, 0x42) };
    const auto packet = stream::frame_message(parts);
    std::span<const uint8_t> buffer{ packet };

    uint8_t flags{};
    std::span<const uint8_t> body{};
    BC_REQUIRE(stream::frame_decode(flags, body, buffer));
    BC_REQUIRE_EQUAL(flags, stream::flag_more);
    BC_REQUIRE_EQUAL(body.size(), 2u);
    BC_REQUIRE(stream::frame_decode(flags, body, buffer));
    BC_REQUIRE_EQUAL(flags, stream::flag_long);
    BC_REQUIRE_EQUAL(body.size(), 300u);
    BC_REQUIRE(buffer.empty());
    BC_REQUIRE(!stream::frame_decode(flags, body, buffer));
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__valid_client__completes)
{
    io::context service{};
    peer_socket server_socket{ service };
    peer_socket client_socket{ service };
    connect_pair(service, server_socket, client_socket);

    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    network::zmtp::cipher::key client_secret{};
    network::zmtp::cipher::key client_public{};
    system::x25519::generate(server_secret, server_public);
    system::x25519::generate(client_secret, client_public);
    const context curve{ system::to_chunk(server_secret) };
    BC_REQUIRE(curve.curve());
    BC_REQUIRE_EQUAL(curve.public_key(), server_public);

    stream server{ std::move(server_socket), curve, role::publisher };
    network::zmtp::cipher client{ client_secret, client_public, server_public };
    curve_peer peer{ client_socket, client };

    code server_result{ error::would_block };
    code peer_result{ error::would_block };
    server.async_handshake(true, [&](const code& ec) { server_result = ec; });
    peer.handshake(peer_result);
    service.run();
    BC_REQUIRE_MESSAGE(!server_result, server_result.message());
    BC_REQUIRE_MESSAGE(!peer_result, peer_result.message());
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__null_client__error)
{
    io::context service{};
    peer_socket server_socket{ service };
    peer_socket client_socket{ service };
    connect_pair(service, server_socket, client_socket);

    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    system::x25519::generate(server_secret, server_public);
    const context curve{ system::to_chunk(server_secret) };

    stream server{ std::move(server_socket), curve, role::publisher };
    raw_peer peer{ client_socket, 1 };

    code server_result{ error::would_block };
    code peer_result{ error::would_block };
    server.async_handshake(true, [&](const code& ec) { server_result = ec; });
    peer.handshake(peer_result);
    service.run();
    BC_REQUIRE_EQUAL(server_result, code(error::protocol_violation));
}

BC_AUTO_TEST_CASE(zmtp_stream__context__wrong_size_secret__null_mechanism)
{
    const context short_secret{ data_chunk(31, 0x01) };
    BC_REQUIRE(!short_secret.curve());

    const context none{};
    BC_REQUIRE(!none.curve());
}

BC_AUTO_TEST_CASE(zmtp_stream__curve__subscribe_and_publish__unboxed_both_ways)
{
    io::context service{};
    peer_socket server_socket{ service };
    peer_socket client_socket{ service };
    connect_pair(service, server_socket, client_socket);

    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    network::zmtp::cipher::key client_secret{};
    network::zmtp::cipher::key client_public{};
    system::x25519::generate(server_secret, server_public);
    system::x25519::generate(client_secret, client_public);
    const context curve{ system::to_chunk(server_secret) };

    stream server{ std::move(server_socket), curve, role::publisher };
    network::zmtp::cipher client{ client_secret, client_public, server_public };
    curve_peer peer{ client_socket, client };

    code server_result{ error::would_block };
    code peer_result{ error::would_block };
    server.async_handshake(true, [&](const code& ec) { server_result = ec; });
    peer.handshake(peer_result);
    service.run();
    service.restart();
    BC_REQUIRE(!server_result && !peer_result);

    // The boxed SUBSCRIBE surfaces as a plain command frame.
    stream::frame frame{};
    code read_result{ error::would_block };
    server.async_read_frame(frame, 1024, [&](const code& ec, size_t) { read_result = ec; });
    const data_chunk topic{ 0x68, 0x61, 0x73, 0x68 };
    peer.subscribe(topic);
    service.run();
    service.restart();
    BC_REQUIRE_MESSAGE(!read_result, read_result.message());
    BC_REQUIRE(frame.command());
    BC_REQUIRE(!frame.more());
    BC_REQUIRE_EQUAL(frame.body, base16_chunk("0953554253435249424568617368"));

    // The framed message is boxed per frame and unboxed by the peer.
    const data_stack parts{ topic, data_chunk(300, 0x42), { 0x01, 0x00, 0x00, 0x00 } };
    const auto packet = stream::frame_message(parts);
    code write_result{ error::would_block };
    code message_result{ error::would_block };
    std::vector<data_chunk> received{};
    server.async_write({ packet.data(), packet.size() }, [&](const code& ec, size_t) { write_result = ec; });
    peer.read_message(received, message_result);
    service.run();
    BC_REQUIRE_MESSAGE(!write_result, write_result.message());
    BC_REQUIRE_MESSAGE(!message_result, message_result.message());
    BC_REQUIRE_EQUAL(received.size(), 3u);
    BC_REQUIRE_EQUAL(received.at(0), parts.at(0));
    BC_REQUIRE_EQUAL(received.at(1), parts.at(1));
    BC_REQUIRE_EQUAL(received.at(2), parts.at(2));
}

// CURVE client authorization.
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__context__clients__authorized_expected)
{
    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    network::zmtp::cipher::key listed_secret{};
    network::zmtp::cipher::key listed_public{};
    network::zmtp::cipher::key other_secret{};
    network::zmtp::cipher::key other_public{};
    system::x25519::generate(server_secret, server_public);
    system::x25519::generate(listed_secret, listed_public);
    system::x25519::generate(other_secret, other_public);

    const context any{ system::to_chunk(server_secret) };
    BC_REQUIRE(any.curve());
    BC_REQUIRE(any.authorized(listed_public));
    BC_REQUIRE(any.authorized(other_public));

    const context listed{ system::to_chunk(server_secret), { system::to_chunk(listed_public) } };
    BC_REQUIRE(listed.curve());
    BC_REQUIRE(listed.authorized(listed_public));
    BC_REQUIRE(!listed.authorized(other_public));

    const context malformed{ system::to_chunk(server_secret), { data_chunk(31, 0x01) } };
    BC_REQUIRE(!malformed.curve());
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__authorized_client__completes)
{
    io::context service{};
    peer_socket server_socket{ service };
    peer_socket client_socket{ service };
    connect_pair(service, server_socket, client_socket);

    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    network::zmtp::cipher::key client_secret{};
    network::zmtp::cipher::key client_public{};
    system::x25519::generate(server_secret, server_public);
    system::x25519::generate(client_secret, client_public);
    const context curve{ system::to_chunk(server_secret), { system::to_chunk(client_public) } };
    BC_REQUIRE(curve.curve());

    stream server{ std::move(server_socket), curve, role::publisher };
    network::zmtp::cipher client{ client_secret, client_public, server_public };
    curve_peer peer{ client_socket, client };

    code server_result{ error::would_block };
    code peer_result{ error::would_block };
    server.async_handshake(true, [&](const code& ec) { server_result = ec; });
    peer.handshake(peer_result);
    service.run();
    BC_REQUIRE_MESSAGE(!server_result, server_result.message());
    BC_REQUIRE_MESSAGE(!peer_result, peer_result.message());
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__unauthorized_client__error)
{
    io::context service{};
    peer_socket server_socket{ service };
    peer_socket client_socket{ service };
    connect_pair(service, server_socket, client_socket);

    network::zmtp::cipher::key server_secret{};
    network::zmtp::cipher::key server_public{};
    network::zmtp::cipher::key client_secret{};
    network::zmtp::cipher::key client_public{};
    network::zmtp::cipher::key listed_secret{};
    network::zmtp::cipher::key listed_public{};
    system::x25519::generate(server_secret, server_public);
    system::x25519::generate(client_secret, client_public);
    system::x25519::generate(listed_secret, listed_public);
    const context curve{ system::to_chunk(server_secret), { system::to_chunk(listed_public) } };

    stream server{ std::move(server_socket), curve, role::publisher };
    network::zmtp::cipher client{ client_secret, client_public, server_public };
    curve_peer peer{ client_socket, client };

    code server_result{ error::would_block };
    code peer_result{ error::would_block };
    server.async_handshake(true, [&](const code& ec) { server_result = ec; });
    peer.handshake(peer_result);
    service.run();
    BC_REQUIRE_EQUAL(server_result, code(error::protocol_violation));
    BC_REQUIRE_EQUAL(peer_result, code(error::protocol_violation));
}

// Codec failures.
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__frame_message__empty__empty)
{
    BC_REQUIRE(stream::frame_message({}).empty());
}

BC_AUTO_TEST_CASE(zmtp_stream__parse_greeting__short__false)
{
    auto greeting = stream::make_greeting(false, false);
    greeting.pop_back();

    uint8_t minor{};
    bool curve{};
    bool as_server{};
    BC_REQUIRE(!stream::parse_greeting(greeting, minor, curve, as_server));
}

BC_AUTO_TEST_CASE(zmtp_stream__frame_decode__truncated_long_length__false)
{
    const data_chunk packet{ stream::flag_long, 0x00, 0x00, 0x00 };
    std::span<const uint8_t> buffer{ packet };
    uint8_t flags{};
    std::span<const uint8_t> body{};
    BC_REQUIRE(!stream::frame_decode(flags, body, buffer));
}

BC_AUTO_TEST_CASE(zmtp_stream__frame_decode__missing_short_length__false)
{
    const data_chunk packet{ 0x00 };
    std::span<const uint8_t> buffer{ packet };
    uint8_t flags{};
    std::span<const uint8_t> body{};
    BC_REQUIRE(!stream::frame_decode(flags, body, buffer));
}

BC_AUTO_TEST_CASE(zmtp_stream__frame_decode__truncated_body__false)
{
    const data_chunk packet{ 0x00, 0x05, 0x01 };
    std::span<const uint8_t> buffer{ packet };
    uint8_t flags{};
    std::span<const uint8_t> body{};
    BC_REQUIRE(!stream::frame_decode(flags, body, buffer));
}

BC_AUTO_TEST_CASE(zmtp_stream__command_name__empty__false)
{
    const data_chunk frame{};
    std::string name{};
    std::span<const uint8_t> body{};
    BC_REQUIRE(!stream::command_name(name, body, frame));
}

BC_AUTO_TEST_CASE(zmtp_stream__command_name__truncated_name__false)
{
    const data_chunk frame{ 0x05, 'R', 'E' };
    std::string name{};
    std::span<const uint8_t> body{};
    BC_REQUIRE(!stream::command_name(name, body, frame));
}

BC_AUTO_TEST_CASE(zmtp_stream__ready_property__truncated_name__false)
{
    const data_chunk metadata{ 0x0b, 'S', 'o', 'c' };
    std::string value{};
    BC_REQUIRE(!stream::ready_property("Socket-Type", value, metadata));
}

BC_AUTO_TEST_CASE(zmtp_stream__ready_property__truncated_value_length__false)
{
    const data_chunk metadata{ 0x01, 'a', 0x00, 0x00 };
    std::string value{};
    BC_REQUIRE(!stream::ready_property("a", value, metadata));
}

BC_AUTO_TEST_CASE(zmtp_stream__ready_property__truncated_value__false)
{
    const data_chunk metadata{ 0x01, 'a', 0x00, 0x00, 0x00, 0x05, 'x' };
    std::string value{};
    BC_REQUIRE(!stream::ready_property("a", value, metadata));
}

BC_AUTO_TEST_CASE(zmtp_stream__ready_property__second_property__expected)
{
    const auto metadata = system::splice(stream::make_property("Socket-Type", "SUB"), stream::make_property("Identity", "abc"));
    std::string value{};
    BC_REQUIRE(stream::ready_property("Identity", value, metadata));
    BC_REQUIRE_EQUAL(value, "abc");
}

// Properties.
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__next_layer__connected__open)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    const auto& constant = instance;
    BC_REQUIRE(instance.next_layer().is_open());
    BC_REQUIRE(constant.next_layer().is_open());
    BC_REQUIRE(instance.get_executor() == instance.next_layer().get_executor());
}

// NULL handshake failures.
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__async_handshake__closed_socket__bad_descriptor)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    instance.next_layer().close();
    BC_REQUIRE_EQUAL(shake(service, instance, true), code(error::bad_stream));
}

BC_AUTO_TEST_CASE(zmtp_stream__async_handshake__peer_shutdown__eof)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    REQUIRE_SUCCESS(client, shutdown, io::socket::shutdown_type::send);
    BC_REQUIRE_EQUAL(shake(service, instance, true), end_of_file);
}

BC_AUTO_TEST_CASE(zmtp_stream__async_handshake__bad_signature__protocol_error)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    auto greeting = stream::make_greeting(false, false);
    greeting.front() = 0x00;
    test::write(client, io::buffer(greeting));
    BC_REQUIRE_EQUAL(shake(service, instance, true), protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__async_handshake__truncated_greeting__eof)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    const auto greeting = stream::make_greeting(false, false);
    test::write(client, io::buffer(greeting.data(), stream::greeting_stage1));
    REQUIRE_SUCCESS(client, shutdown, io::socket::shutdown_type::send);
    BC_REQUIRE_EQUAL(shake(service, instance, true), end_of_file);
}

BC_AUTO_TEST_CASE(zmtp_stream__async_handshake__unknown_mechanism__protocol_error)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    auto greeting = stream::make_greeting(false, false);
    greeting.at(12) = 'X';
    test::write(client, io::buffer(greeting));
    BC_REQUIRE_EQUAL(shake(service, instance, true), protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__async_handshake__greeting_then_shutdown__eof)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    test::write(client, io::buffer(stream::make_greeting(false, false)));
    REQUIRE_SUCCESS(client, shutdown, io::socket::shutdown_type::send);
    BC_REQUIRE_EQUAL(shake(service, instance, true), end_of_file);
}

BC_AUTO_TEST_CASE(zmtp_stream__async_handshake__message_before_ready__protocol_error)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    const auto message = stream::frame_encode(data_chunk{ 0x01 }, false, false);
    test::write(client, io::buffer(system::splice(stream::make_greeting(false, false), message)));
    BC_REQUIRE_EQUAL(shake(service, instance, true), protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__async_handshake__ping_before_ready__protocol_error)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    test::write(client, io::buffer(system::splice(stream::make_greeting(false, false), command("PING", {}))));
    BC_REQUIRE_EQUAL(shake(service, instance, true), protocol_error);
}

// NULL frame read failures.
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__async_read_frame__peer_shutdown__eof)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    test::write(client, io::buffer(system::splice(stream::make_greeting(false, false), ready("SUB"))));
    BC_REQUIRE(!shake(service, instance, true));

    REQUIRE_SUCCESS(client, shutdown, io::socket::shutdown_type::send);
    stream::frame frame{};
    BC_REQUIRE_EQUAL(read_one(service, instance, frame), end_of_file);
}

BC_AUTO_TEST_CASE(zmtp_stream__async_read_frame__flags_then_shutdown__eof)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    test::write(client, io::buffer(system::splice(stream::make_greeting(false, false), ready("SUB"))));
    BC_REQUIRE(!shake(service, instance, true));

    test::write(client, io::buffer(data_chunk{ 0x00 }));
    REQUIRE_SUCCESS(client, shutdown, io::socket::shutdown_type::send);
    stream::frame frame{};
    BC_REQUIRE_EQUAL(read_one(service, instance, frame), end_of_file);
}

BC_AUTO_TEST_CASE(zmtp_stream__async_read_frame__above_maximum__message_size)
{
    io::context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const context configuration{};
    stream instance{ std::move(server), configuration, role::publisher };
    test::write(client, io::buffer(system::splice(stream::make_greeting(false, false), ready("SUB"))));
    BC_REQUIRE(!shake(service, instance, true));

    test::write(client, io::buffer(stream::frame_encode(data_chunk(1025, 0x42), false, false)));
    stream::frame frame{};
    BC_REQUIRE_EQUAL(read_one(service, instance, frame), code(error::oversized_payload));
}

// CURVE handshake failures.
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__greeting_then_shutdown__eof)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    test::write(setup.client_socket, io::buffer(stream::make_greeting(false, true)));
    REQUIRE_SUCCESS(setup.client_socket, shutdown, io::socket::shutdown_type::send);
    BC_REQUIRE_EQUAL(shake(setup.service, server, true), end_of_file);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__ping_before_hello__protocol_error)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    test::write(setup.client_socket, io::buffer(system::splice(stream::make_greeting(false, true), command("PING", {}))));
    BC_REQUIRE_EQUAL(shake(setup.service, server, true), protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__empty_hello__protocol_error)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    test::write(setup.client_socket, io::buffer(system::splice(stream::make_greeting(false, true), command("HELLO", {}))));
    BC_REQUIRE_EQUAL(shake(setup.service, server, true), protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__hello_then_shutdown__eof)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    data_chunk hello{};
    BC_REQUIRE(client.hello(hello));
    test::write(setup.client_socket, io::buffer(system::splice(stream::make_greeting(false, true), stream::frame_encode(hello, true, false))));
    REQUIRE_SUCCESS(setup.client_socket, shutdown, io::socket::shutdown_type::send);
    BC_REQUIRE_EQUAL(shake(setup.service, server, true), end_of_file);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__ping_before_initiate__protocol_error)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    data_chunk hello{};
    BC_REQUIRE(client.hello(hello));
    test::write(setup.client_socket, io::buffer(system::build_chunk({ stream::make_greeting(false, true), stream::frame_encode(hello, true, false), command("PING", {}) })));
    BC_REQUIRE_EQUAL(shake(setup.service, server, true), protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__empty_initiate__protocol_error)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    data_chunk hello{};
    BC_REQUIRE(client.hello(hello));
    test::write(setup.client_socket, io::buffer(system::build_chunk({ stream::make_greeting(false, true), stream::frame_encode(hello, true, false), command("INITIATE", {}) })));
    BC_REQUIRE_EQUAL(shake(setup.service, server, true), protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_handshake__incompatible_socket_type__protocol_error)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    curve_peer peer{ setup.client_socket, client, "PUB" };

    code server_result{ error::would_block };
    code peer_result{ error::would_block };
    server.async_handshake(true, [&](const code& ec) { server_result = ec; });
    peer.handshake(peer_result);
    setup.service.run();
    BC_REQUIRE_EQUAL(server_result, protocol_error);
    BC_REQUIRE_EQUAL(peer_result, protocol_error);
}

// CURVE frames.
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(zmtp_stream__curve_read_frame__empty_frame__protocol_error)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    curve_peer peer{ setup.client_socket, client };
    code peer_result{ error::would_block };
    peer.handshake(peer_result);
    BC_REQUIRE(!shake(setup.service, server, true));
    BC_REQUIRE(!peer_result);

    test::write(setup.client_socket, io::buffer(stream::frame_encode(data_chunk{}, false, false)));
    stream::frame frame{};
    BC_REQUIRE_EQUAL(read_one(setup.service, server, frame), protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_read_frame__plain_error_command__surfaced)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    curve_peer peer{ setup.client_socket, client };
    code peer_result{ error::would_block };
    peer.handshake(peer_result);
    BC_REQUIRE(!shake(setup.service, server, true));
    BC_REQUIRE(!peer_result);

    test::write(setup.client_socket, io::buffer(stream::make_error("abc")));
    stream::frame frame{};
    BC_REQUIRE(!read_one(setup.service, server, frame));
    BC_REQUIRE(frame.command());

    std::string name{};
    std::span<const uint8_t> content{};
    const std::span<const uint8_t> body{ frame.body };
    BC_REQUIRE(stream::command_name(name, content, body));
    BC_REQUIRE_EQUAL(name, "ERROR");
    BC_REQUIRE_EQUAL(data_chunk(content.begin(), content.end()), base16_chunk("03616263"));
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_read_frame__plain_ping_command__protocol_error)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    curve_peer peer{ setup.client_socket, client };
    code peer_result{ error::would_block };
    peer.handshake(peer_result);
    BC_REQUIRE(!shake(setup.service, server, true));
    BC_REQUIRE(!peer_result);

    test::write(setup.client_socket, io::buffer(command("PING", { 0x00, 0x00 })));
    stream::frame frame{};
    BC_REQUIRE_EQUAL(read_one(setup.service, server, frame), protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_read_frame__boxed_more__more_flag)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    curve_peer peer{ setup.client_socket, client };
    code peer_result{ error::would_block };
    peer.handshake(peer_result);
    BC_REQUIRE(!shake(setup.service, server, true));
    BC_REQUIRE(!peer_result);

    const data_chunk part{ 0x01, 0x02 };
    peer.send(network::zmtp::cipher::payload_more, part);
    stream::frame frame{};
    BC_REQUIRE(!read_one(setup.service, server, frame));
    BC_REQUIRE(frame.more());
    BC_REQUIRE(!frame.command());
    BC_REQUIRE_EQUAL(frame.body, part);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_async_write__truncated_frame__protocol_error)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    curve_peer peer{ setup.client_socket, client };
    code peer_result{ error::would_block };
    peer.handshake(peer_result);
    BC_REQUIRE(!shake(setup.service, server, true));
    BC_REQUIRE(!peer_result);

    const data_chunk packet{ 0x00 };
    code result{ error::would_block };
    server.async_write({ packet.data(), packet.size() }, [&](const code& ec, size_t) { result = ec; });
    setup.service.run();
    BC_REQUIRE_EQUAL(result, protocol_error);
}

BC_AUTO_TEST_CASE(zmtp_stream__curve_async_write__command_frame__boxed_command_payload)
{
    curve_setup setup{};
    const context curve{ system::to_chunk(setup.server_secret) };
    stream server{ std::move(setup.server_socket), curve, role::publisher };
    network::zmtp::cipher client{ setup.client_secret, setup.client_public, setup.server_public };
    curve_peer peer{ setup.client_socket, client };
    code peer_result{ error::would_block };
    peer.handshake(peer_result);
    BC_REQUIRE(!shake(setup.service, server, true));
    BC_REQUIRE(!peer_result);

    const auto pong = stream::make_pong(data_chunk{ 0x2a });
    code result{ error::would_block };
    server.async_write({ pong.data(), pong.size() }, [&](const code& ec, size_t) { result = ec; });

    code received{ error::would_block };
    data_chunk boxed{};
    peer.read_frame([&](const code& ec, uint8_t, const data_chunk& message)
    {
        received = ec;
        boxed = message;
    });

    setup.service.run();
    BC_REQUIRE(!result);
    BC_REQUIRE(!received);

    uint8_t payload{};
    data_chunk body{};
    BC_REQUIRE(client.decode(payload, body, boxed));
    BC_REQUIRE_EQUAL(payload, network::zmtp::cipher::payload_command);
    BC_REQUIRE_EQUAL(body, base16_chunk("04504f4e472a"));
}

BC_AUTO_TEST_SUITE_END()
