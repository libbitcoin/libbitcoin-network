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

BOOST_AUTO_TEST_SUITE(zmtp_cipher_tests)

using namespace network::zmtp;
using system::data_chunk;
using system::base16_array;
using system::base16_chunk;

// Test infrastructure (a client and server pair with fresh long-term keys).
// ----------------------------------------------------------------------------

struct cipher_pair
{
    cipher_pair()
    {
        system::x25519::generate(server_secret, server_public);
        system::x25519::generate(client_secret, client_public);
        server = std::make_unique<cipher>(server_secret, server_public);
        client = std::make_unique<cipher>(client_secret, client_public,
            server_public);
    }

    // Run the complete handshake with the given client metadata.
    bool handshake(const data_chunk& client_metadata,
        const data_chunk& server_metadata)
    {
        return client->hello(hello) &&
            server->welcome(welcome, hello) &&
            client->initiate(initiate, welcome, client_metadata) &&
            server->ready(ready, client_metadata_out, initiate,
                server_metadata) &&
            client->complete(server_metadata_out, ready);
    }

    cipher::key server_secret{};
    cipher::key server_public{};
    cipher::key client_secret{};
    cipher::key client_public{};
    std::unique_ptr<cipher> server{};
    std::unique_ptr<cipher> client{};

    data_chunk hello{};
    data_chunk welcome{};
    data_chunk initiate{};
    data_chunk ready{};
    data_chunk client_metadata_out{};
    data_chunk server_metadata_out{};
};

static data_chunk socket_type(const std::string& type)
{
    return stream::make_property("Socket-Type", type);
}

// Keys.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(zmtp_cipher__to_public__rfc7748_alice__expected)
{
    const auto secret = base16_array("77076d0a7318a57d3c16c17251b26645df4c2f87ebc0992ab177fba51db92c2a");
    const auto expected = base16_array("8520f0098930a754748b7ddcb43ef75a0dbf3a0d26381af4eba4a98eaa9b4e6a");

    cipher::key out{};
    BOOST_REQUIRE(cipher::to_public(out, secret));
    BOOST_REQUIRE_EQUAL(out, expected);
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__generate__twice__distinct_valid_keys)
{
    cipher::key secret1{};
    cipher::key public1{};
    cipher::key secret2{};
    cipher::key public2{};
    system::x25519::generate(secret1, public1);
    system::x25519::generate(secret2, public2);
    BOOST_REQUIRE_NE(secret1, secret2);
    BOOST_REQUIRE_NE(public1, public2);

    cipher::key derived{};
    BOOST_REQUIRE(cipher::to_public(derived, secret1));
    BOOST_REQUIRE_EQUAL(derived, public1);
}

// Handshake.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(zmtp_cipher__handshake__valid__command_sizes_and_metadata)
{
    cipher_pair pair{};
    const auto client_metadata = socket_type("SUB");
    const auto server_metadata = socket_type("PUB");
    BOOST_REQUIRE(pair.handshake(client_metadata, server_metadata));

    BOOST_REQUIRE_EQUAL(pair.hello.size(), cipher::hello_size);
    BOOST_REQUIRE_EQUAL(pair.welcome.size(), cipher::welcome_size);
    BOOST_REQUIRE_EQUAL(pair.initiate.size(), cipher::initiate_minimum + client_metadata.size());
    BOOST_REQUIRE_EQUAL(pair.ready.size(), cipher::ready_minimum + server_metadata.size());
    BOOST_REQUIRE_EQUAL(pair.client_metadata_out, client_metadata);
    BOOST_REQUIRE_EQUAL(pair.server_metadata_out, server_metadata);
    BOOST_REQUIRE_EQUAL(pair.server->peer_key(), pair.client_public);
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__handshake__command_names__expected)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));

    BOOST_REQUIRE_EQUAL(pair.hello.at(0), 5u);
    BOOST_REQUIRE_EQUAL(std::string(std::next(pair.hello.begin()), std::next(pair.hello.begin(), 6)), "HELLO");
    BOOST_REQUIRE_EQUAL(pair.hello.at(6), 1u);
    BOOST_REQUIRE_EQUAL(pair.hello.at(7), 0u);
    BOOST_REQUIRE_EQUAL(pair.welcome.at(0), 7u);
    BOOST_REQUIRE_EQUAL(std::string(std::next(pair.welcome.begin()), std::next(pair.welcome.begin(), 8)), "WELCOME");
    BOOST_REQUIRE_EQUAL(pair.initiate.at(0), 8u);
    BOOST_REQUIRE_EQUAL(std::string(std::next(pair.initiate.begin()), std::next(pair.initiate.begin(), 9)), "INITIATE");
    BOOST_REQUIRE_EQUAL(pair.ready.at(0), 5u);
    BOOST_REQUIRE_EQUAL(std::string(std::next(pair.ready.begin()), std::next(pair.ready.begin(), 6)), "READY");
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__handshake__hello_nonce_one_initiate_two__expected)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));

    // Short nonces are big-endian (as libzmq), HELLO at 112, INITIATE at 105.
    const data_chunk hello_nonce(std::next(pair.hello.begin(), 112), std::next(pair.hello.begin(), 120));
    const data_chunk initiate_nonce(std::next(pair.initiate.begin(), 105), std::next(pair.initiate.begin(), 113));
    BOOST_REQUIRE_EQUAL(hello_nonce, base16_chunk("0000000000000001"));
    BOOST_REQUIRE_EQUAL(initiate_nonce, base16_chunk("0000000000000002"));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__welcome__wrong_server_key__false)
{
    cipher_pair pair{};

    // The client believes in a different server key.
    cipher::key other_secret{};
    cipher::key other_public{};
    system::x25519::generate(other_secret, other_public);
    cipher client{ pair.client_secret, pair.client_public, other_public };
    BOOST_REQUIRE(client.hello(pair.hello));
    BOOST_REQUIRE(!pair.server->welcome(pair.welcome, pair.hello));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__welcome__wrong_size__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.client->hello(pair.hello));
    pair.hello.pop_back();
    BOOST_REQUIRE(!pair.server->welcome(pair.welcome, pair.hello));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__welcome__tampered_box__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.client->hello(pair.hello));
    pair.hello.back() ^= 0x01;
    BOOST_REQUIRE(!pair.server->welcome(pair.welcome, pair.hello));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__initiate__tampered_welcome__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.client->hello(pair.hello));
    BOOST_REQUIRE(pair.server->welcome(pair.welcome, pair.hello));
    pair.welcome.back() ^= 0x01;
    BOOST_REQUIRE(!pair.client->initiate(pair.initiate, pair.welcome, {}));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__ready__foreign_cookie__false)
{
    // A cookie from another connection to the same server is rejected.
    cipher_pair pair{};
    cipher_pair other{};
    cipher other_server{ pair.server_secret, pair.server_public };
    cipher other_client{ other.client_secret, other.client_public, pair.server_public };
    BOOST_REQUIRE(other_client.hello(other.hello));
    BOOST_REQUIRE(other_server.welcome(other.welcome, other.hello));
    BOOST_REQUIRE(other_client.initiate(other.initiate, other.welcome, {}));

    BOOST_REQUIRE(pair.client->hello(pair.hello));
    BOOST_REQUIRE(pair.server->welcome(pair.welcome, pair.hello));
    BOOST_REQUIRE(!pair.server->ready(pair.ready, pair.client_metadata_out, other.initiate, {}));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__ready__tampered_initiate__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.client->hello(pair.hello));
    BOOST_REQUIRE(pair.server->welcome(pair.welcome, pair.hello));
    BOOST_REQUIRE(pair.client->initiate(pair.initiate, pair.welcome, {}));
    pair.initiate.back() ^= 0x01;
    BOOST_REQUIRE(!pair.server->ready(pair.ready, pair.client_metadata_out, pair.initiate, {}));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__ready__replayed_initiate__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));
    BOOST_REQUIRE(!pair.server->ready(pair.ready, pair.client_metadata_out, pair.initiate, {}));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__complete__tampered_ready__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.client->hello(pair.hello));
    BOOST_REQUIRE(pair.server->welcome(pair.welcome, pair.hello));
    BOOST_REQUIRE(pair.client->initiate(pair.initiate, pair.welcome, {}));
    BOOST_REQUIRE(pair.server->ready(pair.ready, pair.client_metadata_out, pair.initiate, {}));
    pair.ready.back() ^= 0x01;
    BOOST_REQUIRE(!pair.client->complete(pair.server_metadata_out, pair.ready));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__handshake__role_misuse__false)
{
    cipher_pair pair{};
    data_chunk out{};
    data_chunk metadata{};
    BOOST_REQUIRE(!pair.server->hello(out));
    BOOST_REQUIRE(!pair.client->welcome(out, {}));
    BOOST_REQUIRE(!pair.client->ready(out, metadata, {}, {}));
    BOOST_REQUIRE(!pair.server->initiate(out, {}, {}));
    BOOST_REQUIRE(!pair.server->complete(metadata, {}));
}

// Messages.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(zmtp_cipher__encode__before_handshake__false)
{
    cipher_pair pair{};
    data_chunk out{};
    BOOST_REQUIRE(!pair.server->encode(out, 0x00, {}));
    BOOST_REQUIRE(!pair.client->encode(out, 0x00, {}));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__encode_decode__both_directions__round_trip)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));

    const auto body = base16_chunk("00112233445566778899aabbccddeeff");
    data_chunk message{};
    BOOST_REQUIRE(pair.server->encode(message, cipher::payload_more, body));
    BOOST_REQUIRE_EQUAL(message.size(), cipher::message_minimum + body.size());
    BOOST_REQUIRE_EQUAL(message.at(0), 7u);
    BOOST_REQUIRE_EQUAL(std::string(std::next(message.begin()), std::next(message.begin(), 8)), "MESSAGE");

    uint8_t flags{};
    data_chunk decoded{};
    BOOST_REQUIRE(pair.client->decode(flags, decoded, message));
    BOOST_REQUIRE_EQUAL(flags, cipher::payload_more);
    BOOST_REQUIRE_EQUAL(decoded, body);

    BOOST_REQUIRE(pair.client->encode(message, cipher::payload_command, body));
    BOOST_REQUIRE(pair.server->decode(flags, decoded, message));
    BOOST_REQUIRE_EQUAL(flags, cipher::payload_command);
    BOOST_REQUIRE_EQUAL(decoded, body);
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__encode__empty_body__round_trip)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));

    data_chunk message{};
    BOOST_REQUIRE(pair.server->encode(message, 0x00, {}));
    BOOST_REQUIRE_EQUAL(message.size(), cipher::message_minimum);

    uint8_t flags{ 0xff };
    data_chunk decoded{ 0x42 };
    BOOST_REQUIRE(pair.client->decode(flags, decoded, message));
    BOOST_REQUIRE_EQUAL(flags, 0x00u);
    BOOST_REQUIRE(decoded.empty());
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__decode__replay__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));

    data_chunk message{};
    BOOST_REQUIRE(pair.server->encode(message, 0x00, data_chunk{ 0x01, 0x02 }));

    uint8_t flags{};
    data_chunk decoded{};
    BOOST_REQUIRE(pair.client->decode(flags, decoded, message));
    BOOST_REQUIRE(!pair.client->decode(flags, decoded, message));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__decode__reordered__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));

    data_chunk first{};
    data_chunk second{};
    BOOST_REQUIRE(pair.server->encode(first, 0x00, data_chunk{ 0x01 }));
    BOOST_REQUIRE(pair.server->encode(second, 0x00, data_chunk{ 0x02 }));

    uint8_t flags{};
    data_chunk decoded{};
    BOOST_REQUIRE(pair.client->decode(flags, decoded, second));
    BOOST_REQUIRE(!pair.client->decode(flags, decoded, first));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__decode__wrong_direction__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));

    // A message boxed by the server is not accepted by the server.
    data_chunk message{};
    BOOST_REQUIRE(pair.server->encode(message, 0x00, data_chunk{ 0x01 }));

    uint8_t flags{};
    data_chunk decoded{};
    BOOST_REQUIRE(!pair.server->decode(flags, decoded, message));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__decode__tampered__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));

    data_chunk message{};
    BOOST_REQUIRE(pair.server->encode(message, 0x00, data_chunk{ 0x01, 0x02, 0x03 }));
    message.back() ^= 0x01;

    uint8_t flags{};
    data_chunk decoded{};
    BOOST_REQUIRE(!pair.client->decode(flags, decoded, message));
}

BOOST_AUTO_TEST_CASE(zmtp_cipher__decode__truncated__false)
{
    cipher_pair pair{};
    BOOST_REQUIRE(pair.handshake({}, {}));

    data_chunk message{};
    BOOST_REQUIRE(pair.server->encode(message, 0x00, {}));
    message.pop_back();

    uint8_t flags{};
    data_chunk decoded{};
    BOOST_REQUIRE(!pair.client->decode(flags, decoded, message));
}

BOOST_AUTO_TEST_SUITE_END()
