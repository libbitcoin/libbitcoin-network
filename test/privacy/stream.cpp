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

BOOST_AUTO_TEST_SUITE(privacy_stream_tests)

using v2_stream = network::privacy::stream;
using v2_context = network::privacy::context;
using tcp_socket = network::asio::socket;
using network::messages::peer::heading;
namespace identifiers = network::messages::peer::identifiers;
using system::data_chunk;

constexpr uint32_t mainnet = 0xd9b4bef9;

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

BOOST_AUTO_TEST_CASE(privacy_stream__handshake__v2_both_sides__frames_round_trip)
{
    boost::asio::io_context service{};
    tcp_socket server{ service };
    tcp_socket client{ service };
    connect_pair(service, server, client);

    const v2_context configuration{ mainnet };
    v2_stream initiator{ std::move(client), configuration };

    // The initiator sends its key, the socket detects v2 (not v1 prefix).
    boost_code initiated{ boost::asio::error::would_block };
    initiator.async_handshake(true, [&](const boost_code& ec) { initiated = ec; });

    data_chunk prefix(v2_stream::detection_size);
    boost_code detected{ boost::asio::error::would_block };
    const auto detect = [&](const boost_code& ec, size_t) { detected = ec; };
    const boost::asio::mutable_buffer out{ prefix.data(), prefix.size() };
    boost::asio::async_read(server, out, detect);

    service.run();
    service.restart();
    BOOST_REQUIRE(!detected);
    BOOST_REQUIRE(!v2_stream::detected_v1(prefix, mainnet));

    // The responder stream assumes the detection prefix (partial peer key).
    v2_stream responder{ std::move(server), configuration, std::move(prefix) };

    boost_code responded{ boost::asio::error::would_block };
    responder.async_handshake(false, [&](const boost_code& ec) { responded = ec; });
    service.run();
    service.restart();

    BOOST_REQUIRE(!initiated);
    BOOST_REQUIRE(!responded);
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
        stream.async_read_message(buffer, on_message);
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

BOOST_AUTO_TEST_CASE(privacy_stream__detected_v1__version_prefix__true)
{
    // A v1 peer opens with a version message.
    const auto payload = system::base16_chunk("00112233445566778899");
    const auto version = v1_frame("version", payload);
    const auto end = std::next(version.begin(), v2_stream::detection_size);
    const data_chunk prefix(version.begin(), end);

    BOOST_REQUIRE(v2_stream::detected_v1(prefix, mainnet));
}

BOOST_AUTO_TEST_CASE(privacy_stream__detected_v1__wrong_magic__false)
{
    const auto payload = system::base16_chunk("00112233445566778899");
    const auto version = v1_frame("version", payload);
    const auto end = std::next(version.begin(), v2_stream::detection_size);
    const data_chunk prefix(version.begin(), end);

    BOOST_REQUIRE(!v2_stream::detected_v1(prefix, 0x0b110907));
}

BOOST_AUTO_TEST_CASE(privacy_stream__detected_v1__random_key__false)
{
    // An ellswift key cannot match the version prefix.
    const v2_context configuration{ mainnet };
    const data_chunk prefix(v2_stream::detection_size, 0x42);

    BOOST_REQUIRE(!v2_stream::detected_v1(prefix, mainnet));
}

BOOST_AUTO_TEST_SUITE_END()
