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
#include "../../test.hpp"

BOOST_AUTO_TEST_SUITE(p2p_message_tests)

using namespace system;
using namespace network::messages::peer;

constexpr auto empty_checksum = 0xe2e0f65d_u32;
constexpr auto empty_hash = sha256::double_hash(sha256::ablocks_t<0>{});
static_assert(from_little_endian<uint32_t>(empty_hash) == empty_checksum);

BOOST_AUTO_TEST_CASE(message__network_checksum__empty_hash__empty_checksum)
{
    BOOST_REQUIRE_EQUAL(network_checksum(empty_hash), empty_checksum);
}

using registry = messages::peer::registry;

static constexpr uint32_t mainnet_magic = 0xd9b4bef9;
static constexpr uint64_t nonce = 0x0123456789abcdef;
static const auto ping_payload = base16_chunk("efcdab8967452301");
static const auto verack_frame = base16_chunk("f9beb4d976657261636b000000000000000000005df6e0e2");
static const auto genesis_tx = base16_chunk("01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73ffffffff0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac00000000");
static const auto coinbase_840000 = base16_chunk("010000000001010000000000000000000000000000000000000000000000000000000000000000ffffffff600340d10c192f5669614254432f4d696e65642062792062757a7a3132302f2cfabe6d6d144b553283a6e1a150c9989428c0695e3a1bef7d482ed1f829bbe25897fd37dc10000000000000001058a4c9000cc3a31889b38ae08249000000000000ffffffff03fb80e4f2000000001976a914536ffa992491508dca0354e52f32a3a7a679a53a88ac00000000000000002b6a2952534b424c4f434b3a52e15efafb3e2cf6dc2fc0e6bde5cb1d7d2143f1e089bd874e6b7913005fb2a00000000000000000266a24aa21a9ed88601d3d03ccce017fe2131c4c95a7292e4372983148e62996bb5e2de0e4d1d80120000000000000000000000000000000000000000000000000000000000000000000000000");

BOOST_AUTO_TEST_CASE(message__serialize__ping_payload__expected)
{
    const auto data = messages::peer::serialize(ping{ nonce }, level::bip31);
    BOOST_REQUIRE(data);
    BOOST_REQUIRE_EQUAL(*data, ping_payload);
}

BOOST_AUTO_TEST_CASE(message__serialize__verack_frame__expected)
{
    const auto data = messages::peer::serialize(version_acknowledge{}, mainnet_magic, level::minimum_protocol);
    BOOST_REQUIRE(data);
    BOOST_REQUIRE_EQUAL(*data, verack_frame);
}

BOOST_AUTO_TEST_CASE(message__deserialize__ping_payload__expected)
{
    const auto message = messages::peer::deserialize<ping>(ping_payload, level::bip31);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->nonce, nonce);
}

BOOST_AUTO_TEST_CASE(message__serialize__genesis_tx_frame__txid_checksum)
{
    const auto message = transaction::deserialize(level::minimum_protocol, genesis_tx, true);
    BOOST_REQUIRE(message);
    const auto data = messages::peer::serialize(*message, mainnet_magic, level::minimum_protocol);
    BOOST_REQUIRE(data);
    BOOST_REQUIRE_EQUAL(*data, splice(base16_chunk("f9beb4d9747800000000000000000000cc0000003ba3edfd"), genesis_tx));
}

BOOST_AUTO_TEST_CASE(message__serialize__witness_coinbase_840000_frame__wtxid_checksum)
{
    const auto message = transaction::deserialize(level::minimum_protocol, coinbase_840000, true);
    BOOST_REQUIRE(message);
    const auto data = messages::peer::serialize(*message, mainnet_magic, level::minimum_protocol);
    BOOST_REQUIRE(data);
    BOOST_REQUIRE_EQUAL(*data, splice(base16_chunk("f9beb4d97478000000000000000000003c010000c8cada0c"), coinbase_840000));
}

// registry

BOOST_AUTO_TEST_CASE(message__registry_index__bip324_identifiers__expected)
{
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 1 }), registry::index_of<messages::peer::address>());
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 2 }), registry::index_of<messages::peer::block>());
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 13 }), registry::index_of<messages::peer::headers>());
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 14 }), registry::index_of<messages::peer::inventory>());
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 18 }), registry::index_of<messages::peer::ping>());
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 19 }), registry::index_of<messages::peer::pong>());
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 21 }), registry::index_of<messages::peer::transaction>());
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 28 }), registry::index_of<messages::peer::address_v2>());
}

BOOST_AUTO_TEST_CASE(message__registry_index__unassigned_identifier__unknown)
{
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 0 }), registry::unknown);
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 29 }), registry::unknown);
    BOOST_REQUIRE_EQUAL(registry::index(uint8_t{ 255 }), registry::unknown);
}

BOOST_AUTO_TEST_CASE(message__registry_identifiers__version_unassigned__zero)
{
    BOOST_REQUIRE_EQUAL(registry::identifiers().at(registry::index_of<messages::peer::version>()), 0u);
    BOOST_REQUIRE_EQUAL(registry::identifiers().at(registry::index_of<messages::peer::version_acknowledge>()), 0u);
    BOOST_REQUIRE_EQUAL(registry::identifiers().at(registry::index_of<messages::peer::ping>()), 18u);
}

BOOST_AUTO_TEST_CASE(message__registry_to_payload__ping__expected)
{
    const rpc::any_t message{ to_shared(ping{ nonce }) };
    const auto data = registry::to_payload(registry::index_of<messages::peer::ping>(), message, level::bip31);
    BOOST_REQUIRE(data);
    BOOST_REQUIRE_EQUAL(*data, ping_payload);
}

BOOST_AUTO_TEST_CASE(message__registry_to_payload__mismatched_type__nullptr)
{
    const rpc::any_t message{ to_shared(ping{ nonce }) };
    BOOST_REQUIRE(!registry::to_payload(registry::index_of<messages::peer::pong>(), message, level::bip31));
}

BOOST_AUTO_TEST_CASE(message__registry_to_payload__unknown_index__nullptr)
{
    const rpc::any_t message{ to_shared(ping{ nonce }) };
    BOOST_REQUIRE(!registry::to_payload(registry::unknown, message, level::bip31));
}

BOOST_AUTO_TEST_CASE(message__registry_to_frame__verack__expected)
{
    const rpc::any_t message{ to_shared(version_acknowledge{}) };
    const auto data = registry::to_frame(registry::index_of<messages::peer::version_acknowledge>(), message, mainnet_magic, level::minimum_protocol);
    BOOST_REQUIRE(data);
    BOOST_REQUIRE_EQUAL(*data, verack_frame);
}

BOOST_AUTO_TEST_CASE(message__registry_to_frame__unknown_index__nullptr)
{
    const rpc::any_t message{ to_shared(version_acknowledge{}) };
    BOOST_REQUIRE(!registry::to_frame(registry::unknown, message, mainnet_magic, level::minimum_protocol));
}

BOOST_AUTO_TEST_CASE(message__registry_to_any__ping__expected)
{
    const auto any = registry::to_any(registry::index_of<messages::peer::ping>(), ping_payload, level::bip31, true);
    const auto message = any.get<const ping>();
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->nonce, nonce);
}

BOOST_AUTO_TEST_CASE(message__registry_to_any__unknown_index__empty)
{
    BOOST_REQUIRE(!registry::to_any(registry::unknown, ping_payload, level::bip31, true));
}

BOOST_AUTO_TEST_CASE(message__registry_to_any__invalid_payload__empty)
{
    BOOST_REQUIRE(!registry::to_any(registry::index_of<messages::peer::ping>(), base16_chunk("efcd"), level::bip31, true));
}

BOOST_AUTO_TEST_SUITE_END()
