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

BOOST_AUTO_TEST_SUITE(p2ps_stream_tests)

using v2_stream = network::p2ps::stream;
using v2_context = network::p2ps::context;
using peer_socket = network::asio::socket;
using network::messages::peer::heading;
namespace identifiers = network::messages::peer::identifiers;
using system::data_chunk;

constexpr uint32_t mainnet = 0xd9b4bef9;

// Establish a connected local socket pair on the given service.
static void connect_pair(boost::asio::io_context& service,
    peer_socket& server, peer_socket& client)
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

// Serialize a v1 message frame (heading and payload).
static data_chunk v1_frame(const std::string& command,
    const data_chunk& payload)
{
    const auto head = heading::factory(mainnet, command, payload);
    data_chunk frame(heading::size() + payload.size());
    BOOST_REQUIRE(head.serialize({ frame.data(),
        std::next(frame.data(), heading::size()) }));
    std::copy(payload.begin(), payload.end(),
        std::next(frame.begin(), heading::size()));
    return frame;
}

using v2_cipher = network::p2ps::cipher;
typedef std::function<void(v2_cipher&)> responder;

// The raw peer reads the initiator key, initializes its cipher and responds.
static void respond(peer_socket& raw, v2_cipher& peer, const responder& handler)
{
    const auto key = std::make_shared<v2_cipher::key>();
    boost::asio::async_read(raw, boost::asio::buffer(*key), [&peer, key, handler](const boost_code& ec, size_t)
    {
        BOOST_REQUIRE(!ec);
        BOOST_REQUIRE(peer.initialize(*key, mainnet, false));
        handler(peer);
    });
}

// The responder key, garbage, garbage terminator and version packet.
static data_chunk greeting(v2_cipher& peer, const data_chunk& garbage)
{
    data_chunk version(v2_cipher::expansion);
    peer.encrypt({}, garbage, false, version);
    return system::build_chunk({ peer.public_key(), garbage, peer.send_terminator(), version });
}

// An encrypted packet of the given contents.
static data_chunk packet(v2_cipher& peer, const data_chunk& contents, bool ignore)
{
    data_chunk out(contents.size() + v2_cipher::expansion);
    peer.encrypt(contents, {}, ignore, out);
    return out;
}

static boost_code initiate(boost::asio::io_context& service, v2_stream& stream)
{
    boost_code result{ boost::asio::error::would_block };
    stream.async_handshake([&](const boost_code& ec) { result = ec; });
    service.run();
    service.restart();
    return result;
}

// Complete the handshake of the initiator stream against the raw peer.
static void shake(boost::asio::io_context& service, v2_stream& stream, peer_socket& raw, v2_cipher& peer)
{
    respond(raw, peer, [&raw](v2_cipher& self)
    {
        boost::asio::write(raw, boost::asio::buffer(greeting(self, {})));
    });

    BOOST_REQUIRE(!initiate(service, stream));
}

struct received
{
    boost_code ec{ boost::asio::error::would_block };
    uint8_t identifier{ 0xff };
    std::string command{};
    data_chunk payload{};
};

static received read_one(boost::asio::io_context& service, v2_stream& stream, size_t maximum)
{
    received out{};
    data_chunk buffer{};
    stream.async_read_message(buffer, maximum, [&](const boost_code& ec, uint8_t identifier, const std::string& command, const v2_stream::payload_t& payload)
    {
        out.ec = ec;
        out.identifier = identifier;
        out.command = command;
        out.payload.assign(payload.begin(), payload.end());
    });

    service.run();
    service.restart();
    return out;
}

static boost_code write_one(boost::asio::io_context& service, v2_stream& stream, uint8_t identifier, const std::string& command, const system::chunk_cptr& payload)
{
    boost_code result{ boost::asio::error::would_block };
    stream.async_write_message(identifier, command, payload, [&](const boost_code& ec, size_t) { result = ec; });
    service.run();
    service.restart();
    return result;
}

static const boost_code protocol_error{ boost::asio::error::no_protocol_option };
static const boost_code end_of_file{ boost::asio::error::eof };

BOOST_AUTO_TEST_CASE(p2ps_stream__handshake__v2_both_sides__frames_round_trip)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    std::optional<v2_stream> upgraded{};

    boost_code initiated{ boost::asio::error::would_block };
    boost_code responded{ boost::asio::error::would_block };
    boost_code detected{ boost::asio::error::would_block };
    data_chunk prefix(v2_stream::detection_size);
    bool v1{ true };

    // The socket reads the detection prefix and upgrades on a v2 peer,
    // passing the detected bytes to the stream (a partial peer key).
    const auto shook = [&](const boost_code& ec) { responded = ec; };
    const auto detect = [&](const boost_code& ec, size_t)
    {
        detected = ec;
        v1 = v2_stream::detected_v1(prefix, mainnet);
        upgraded.emplace(std::move(server), configuration);
        upgraded->async_handshake(std::move(prefix), shook);
    };

    const auto shake = [&](const boost_code& ec) { initiated = ec; };
    initiator.async_handshake(shake);
    const boost::asio::mutable_buffer out{ prefix.data(), prefix.size() };
    boost::asio::async_read(server, out, detect);

    service.run();
    service.restart();

    BOOST_REQUIRE(!detected);
    BOOST_REQUIRE(!v1);
    BOOST_REQUIRE(!initiated);
    BOOST_REQUIRE(!responded);

    auto& responder = *upgraded;
    BOOST_REQUIRE_EQUAL(initiator.session_id(), responder.session_id());

    // Read helper: one message via the native v2 read.
    data_chunk buffer{};
    boost_code got{};
    uint8_t identifier{};
    std::string command{};
    data_chunk payload{};
    using payload_t = v2_stream::payload_t;
    const auto on_message =
        [&](const boost_code& ec, uint8_t id, std::string type, payload_t data)
        {
            got = ec;
            identifier = id;
            command = type;
            payload.assign(data.begin(), data.end());
        };

    const auto read_message = [&](v2_stream& stream)
    {
        got = boost::asio::error::would_block;
        identifier = 0xff;
        command.clear();
        payload.clear();
        constexpr auto maximum = network::p2ps::cipher::maximum_content;
        stream.async_read_message(buffer, maximum, on_message);
    };

    // Send a short-identifier message (ping) initiator to responder.
    const auto ping_payload = system::base16_chunk("0011223344556677");
    const auto ping_ptr = system::to_shared(ping_payload);
    boost_code sent{ boost::asio::error::would_block };
    const auto on_sent = [&](const boost_code& ec, size_t) { sent = ec; };
    initiator.async_write_message(identifiers::ping, "", ping_ptr, on_sent);

    read_message(responder);
    service.run();
    service.restart();
    BOOST_REQUIRE(!sent);
    BOOST_REQUIRE(!got);
    BOOST_REQUIRE_EQUAL(identifier, identifiers::ping);
    BOOST_REQUIRE(command.empty());
    BOOST_REQUIRE_EQUAL(payload, ping_payload);

    // Send an unmapped command (version, 13 byte type) responder to initiator.
    const auto version_payload = system::base16_chunk("deadbeef");
    const auto version_ptr = system::to_shared(version_payload);
    const auto unassigned = identifiers::unassigned;
    sent = boost::asio::error::would_block;
    responder.async_write_message(unassigned, "version", version_ptr, on_sent);

    read_message(initiator);
    service.run();
    service.restart();
    BOOST_REQUIRE(!sent);
    BOOST_REQUIRE(!got);
    BOOST_REQUIRE_EQUAL(identifier, identifiers::unassigned);
    BOOST_REQUIRE_EQUAL(command, "version");
    BOOST_REQUIRE_EQUAL(payload, version_payload);

    // A second short-identifier message reuses the buffer (inv).
    const auto inv_payload = system::base16_chunk("00");
    const auto inv_ptr = system::to_shared(inv_payload);
    sent = boost::asio::error::would_block;
    initiator.async_write_message(identifiers::inventory, "", inv_ptr, on_sent);

    read_message(responder);
    service.run();
    service.restart();
    BOOST_REQUIRE(!sent);
    BOOST_REQUIRE(!got);
    BOOST_REQUIRE_EQUAL(identifier, identifiers::inventory);
    BOOST_REQUIRE(command.empty());
    BOOST_REQUIRE_EQUAL(payload, inv_payload);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__detected_v1__version_prefix__true)
{
    // A v1 peer opens with a version message.
    const auto payload = system::base16_chunk("00112233445566778899");
    const auto version = v1_frame("version", payload);
    const auto end = std::next(version.begin(), v2_stream::detection_size);
    const data_chunk prefix(version.begin(), end);

    BOOST_REQUIRE(v2_stream::detected_v1(prefix, mainnet));
}

BOOST_AUTO_TEST_CASE(p2ps_stream__detected_v1__wrong_magic__false)
{
    const auto payload = system::base16_chunk("00112233445566778899");
    const auto version = v1_frame("version", payload);
    const auto end = std::next(version.begin(), v2_stream::detection_size);
    const data_chunk prefix(version.begin(), end);

    BOOST_REQUIRE(!v2_stream::detected_v1(prefix, 0x0b110907));
}

BOOST_AUTO_TEST_CASE(p2ps_stream__detected_v1__random_key__false)
{
    // An ellswift key cannot match the version prefix.
    const data_chunk prefix(v2_stream::detection_size, 0x42);

    BOOST_REQUIRE(!v2_stream::detected_v1(prefix, mainnet));
}

// Properties.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(p2ps_stream__next_layer__connected__open)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream instance{ std::move(client), configuration };
    const auto& constant = instance;
    BOOST_REQUIRE(instance.next_layer().is_open());
    BOOST_REQUIRE(constant.next_layer().is_open());
}

// Handshake failures.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(p2ps_stream__async_handshake__closed_socket__bad_descriptor)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    initiator.next_layer().close();
    BOOST_REQUIRE_EQUAL(initiate(service, initiator), boost_code(boost::asio::error::bad_descriptor));
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_handshake__responder_peer_shutdown__eof)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream responder{ std::move(server), configuration };
    client.shutdown(boost::asio::socket_base::shutdown_send);

    boost_code result{ boost::asio::error::would_block };
    responder.async_handshake(data_chunk{}, [&](const boost_code& ec) { result = ec; });
    service.run();
    BOOST_REQUIRE_EQUAL(result, end_of_file);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_handshake__peer_key_then_shutdown__eof)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    respond(server, peer, [&server](v2_cipher& self)
    {
        boost::asio::write(server, boost::asio::buffer(self.public_key()));
        server.shutdown(boost::asio::socket_base::shutdown_send);
    });

    BOOST_REQUIRE_EQUAL(initiate(service, initiator), end_of_file);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_handshake__garbage_without_terminator__protocol_error)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    respond(server, peer, [&server](v2_cipher& self)
    {
        const data_chunk garbage(v2_cipher::maximum_garbage + v2_cipher::terminator_size, 0x00);
        boost::asio::write(server, boost::asio::buffer(system::build_chunk({ self.public_key(), garbage })));
    });

    BOOST_REQUIRE_EQUAL(initiate(service, initiator), protocol_error);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_handshake__terminator_then_shutdown__eof)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    respond(server, peer, [&server](v2_cipher& self)
    {
        boost::asio::write(server, boost::asio::buffer(system::build_chunk({ self.public_key(), self.send_terminator() })));
        server.shutdown(boost::asio::socket_base::shutdown_send);
    });

    BOOST_REQUIRE_EQUAL(initiate(service, initiator), end_of_file);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_handshake__truncated_version__eof)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    respond(server, peer, [&server](v2_cipher& self)
    {
        const auto frame = greeting(self, {});
        const auto size = v2_cipher::key_size + v2_cipher::terminator_size + v2_cipher::length_size;
        boost::asio::write(server, boost::asio::buffer(frame.data(), size));
        server.shutdown(boost::asio::socket_base::shutdown_send);
    });

    BOOST_REQUIRE_EQUAL(initiate(service, initiator), end_of_file);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_handshake__version_without_garbage_aad__protocol_error)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    respond(server, peer, [&server](v2_cipher& self)
    {
        const data_chunk garbage{ 0x42 };
        const auto version = packet(self, {}, false);
        boost::asio::write(server, boost::asio::buffer(system::build_chunk({ self.public_key(), garbage, self.send_terminator(), version })));
    });

    BOOST_REQUIRE_EQUAL(initiate(service, initiator), protocol_error);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_handshake__decoy_before_version__success)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    respond(server, peer, [&server](v2_cipher& self)
    {
        const data_chunk garbage{ 0x42, 0x43 };
        const data_chunk contents{ 0x01, 0x02, 0x03 };
        data_chunk decoy(contents.size() + v2_cipher::expansion);
        self.encrypt(contents, garbage, true, decoy);
        const auto version = packet(self, {}, false);
        boost::asio::write(server, boost::asio::buffer(system::build_chunk({ self.public_key(), garbage, self.send_terminator(), decoy, version })));
    });

    BOOST_REQUIRE(!initiate(service, initiator));
    BOOST_REQUIRE_EQUAL(initiator.session_id(), peer.session_id());
}

// Message read failures.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(p2ps_stream__async_read_message__peer_shutdown__eof)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    shake(service, initiator, server, peer);

    server.shutdown(boost::asio::socket_base::shutdown_send);
    BOOST_REQUIRE_EQUAL(read_one(service, initiator, v2_cipher::maximum_content).ec, end_of_file);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_read_message__above_maximum__protocol_error)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    shake(service, initiator, server, peer);

    data_chunk contents(add1(add1(heading::command_size)), 0x00);
    contents.front() = identifiers::ping;
    boost::asio::write(server, boost::asio::buffer(packet(peer, contents, false)));
    BOOST_REQUIRE_EQUAL(read_one(service, initiator, zero).ec, protocol_error);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_read_message__truncated_packet__eof)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    shake(service, initiator, server, peer);

    const auto encrypted = packet(peer, { identifiers::ping, 0x00 }, false);
    boost::asio::write(server, boost::asio::buffer(encrypted.data(), v2_cipher::length_size));
    server.shutdown(boost::asio::socket_base::shutdown_send);
    BOOST_REQUIRE_EQUAL(read_one(service, initiator, v2_cipher::maximum_content).ec, end_of_file);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_read_message__tampered_tag__protocol_error)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    shake(service, initiator, server, peer);

    auto encrypted = packet(peer, { identifiers::ping, 0x00 }, false);
    encrypted.back() ^= 0x01;
    boost::asio::write(server, boost::asio::buffer(encrypted));
    BOOST_REQUIRE_EQUAL(read_one(service, initiator, v2_cipher::maximum_content).ec, protocol_error);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_read_message__decoy_then_ping__ping)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    shake(service, initiator, server, peer);

    const auto decoy = packet(peer, { 0x00, 0x01, 0x02 }, true);
    const auto ping = packet(peer, { identifiers::ping, 0x2a }, false);
    boost::asio::write(server, boost::asio::buffer(system::build_chunk({ decoy, ping })));

    const auto message = read_one(service, initiator, v2_cipher::maximum_content);
    BOOST_REQUIRE(!message.ec);
    BOOST_REQUIRE_EQUAL(message.identifier, identifiers::ping);
    BOOST_REQUIRE(message.command.empty());
    BOOST_REQUIRE_EQUAL(message.payload, data_chunk{ 0x2a });
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_read_message__empty_contents__protocol_error)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    shake(service, initiator, server, peer);

    boost::asio::write(server, boost::asio::buffer(packet(peer, {}, false)));
    BOOST_REQUIRE_EQUAL(read_one(service, initiator, v2_cipher::maximum_content).ec, protocol_error);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_read_message__truncated_message_type__protocol_error)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    shake(service, initiator, server, peer);

    const data_chunk contents{ 0x00, 'p', 'i', 'n', 'g' };
    boost::asio::write(server, boost::asio::buffer(packet(peer, contents, false)));
    BOOST_REQUIRE_EQUAL(read_one(service, initiator, v2_cipher::maximum_content).ec, protocol_error);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_read_message__nonzero_type_padding__protocol_error)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    shake(service, initiator, server, peer);

    const data_chunk contents{ 0x00, 'p', 'i', 'n', 'g', 0x00, 'x', 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    boost::asio::write(server, boost::asio::buffer(packet(peer, contents, false)));
    BOOST_REQUIRE_EQUAL(read_one(service, initiator, v2_cipher::maximum_content).ec, protocol_error);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_read_message__long_message_type__command)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };
    v2_cipher peer{};
    shake(service, initiator, server, peer);

    const data_chunk contents{ 0x00, 'v', 'e', 'r', 's', 'i', 'o', 'n', 0x00, 0x00, 0x00, 0x00, 0x00, 0x42 };
    boost::asio::write(server, boost::asio::buffer(packet(peer, contents, false)));

    const auto message = read_one(service, initiator, v2_cipher::maximum_content);
    BOOST_REQUIRE(!message.ec);
    BOOST_REQUIRE_EQUAL(message.identifier, 0x00u);
    BOOST_REQUIRE_EQUAL(message.command, "version");
    BOOST_REQUIRE_EQUAL(message.payload, data_chunk{ 0x42 });
}

// Message write failures.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(p2ps_stream__async_write_message__null_payload__protocol_error)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream instance{ std::move(client), configuration };
    BOOST_REQUIRE_EQUAL(write_one(service, instance, identifiers::ping, "", nullptr), protocol_error);
}

BOOST_AUTO_TEST_CASE(p2ps_stream__async_write_message__oversized_command__protocol_error)
{
    boost::asio::io_context service{};
    peer_socket server{ service };
    peer_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream instance{ std::move(client), configuration };
    const auto payload = system::to_shared(data_chunk{ 0x42 });
    BOOST_REQUIRE_EQUAL(write_one(service, instance, identifiers::unassigned, "thirteenbytes", payload), protocol_error);
}

BOOST_AUTO_TEST_SUITE_END()
