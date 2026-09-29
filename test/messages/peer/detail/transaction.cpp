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
#include "../../../test.hpp"

BOOST_AUTO_TEST_SUITE(p2p_transaction_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(transaction__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(transaction::command, "tx");
    constexpr auto index = messages::peer::registry::index_of<transaction>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), transaction::command);
    BOOST_REQUIRE_EQUAL(transaction::version_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(transaction::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(transaction__size__default__zero)
{
    BOOST_REQUIRE_EQUAL(transaction{}.size(level::canonical, true), zero);
    BOOST_REQUIRE_EQUAL(transaction{}.size(level::canonical, false), zero);
}

static const auto genesis_tx_hash = system::base16_hash("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
static const auto genesis_tx = system::base16_chunk("01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73ffffffff0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac00000000");
static const auto coinbase_840000_txid = system::base16_hash("a0db149ace545beabbd87a8d6b20ffd6aa3b5a50e58add49a3d435f898c272cf");
static const auto coinbase_840000_wtxid = system::base16_hash("ba2a2a86efa306684a88b6e0cdb8abf25d9f66e6a851c9b9c67f25640cdacac8");
static const auto coinbase_840000 = system::base16_chunk("010000000001010000000000000000000000000000000000000000000000000000000000000000ffffffff600340d10c192f5669614254432f4d696e65642062792062757a7a3132302f2cfabe6d6d144b553283a6e1a150c9989428c0695e3a1bef7d482ed1f829bbe25897fd37dc10000000000000001058a4c9000cc3a31889b38ae08249000000000000ffffffff03fb80e4f2000000001976a914536ffa992491508dca0354e52f32a3a7a679a53a88ac00000000000000002b6a2952534b424c4f434b3a52e15efafb3e2cf6dc2fc0e6bde5cb1d7d2143f1e089bd874e6b7913005fb2a00000000000000000266a24aa21a9ed88601d3d03ccce017fe2131c4c95a7292e4372983148e62996bb5e2de0e4d1d80120000000000000000000000000000000000000000000000000000000000000000000000000");

BOOST_AUTO_TEST_CASE(transaction__deserialize1__genesis__nominal_hash_cached)
{
    const auto message = transaction::deserialize(level::minimum_protocol, genesis_tx, false);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE(message->transaction_ptr);
    BOOST_REQUIRE(!message->transaction_ptr->is_segregated());
    BOOST_REQUIRE_EQUAL(message->transaction_ptr->get_hash(false), genesis_tx_hash);
    BOOST_REQUIRE_EQUAL(message->size(level::minimum_protocol, false), genesis_tx.size());
}

BOOST_AUTO_TEST_CASE(transaction__deserialize1__witness_coinbase_840000__both_hashes_cached)
{
    const auto message = transaction::deserialize(level::minimum_protocol, coinbase_840000, true);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE(message->transaction_ptr->is_segregated());
    BOOST_REQUIRE_EQUAL(message->transaction_ptr->get_hash(false), coinbase_840000_txid);
    BOOST_REQUIRE_EQUAL(message->transaction_ptr->get_hash(true), coinbase_840000_wtxid);
    BOOST_REQUIRE_EQUAL(message->size(level::minimum_protocol, true), 316u);
    BOOST_REQUIRE_EQUAL(message->size(level::minimum_protocol, true), coinbase_840000.size());
}

BOOST_AUTO_TEST_CASE(transaction__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!transaction::deserialize(level::minimum_protocol - 1u, genesis_tx, false));
}

BOOST_AUTO_TEST_CASE(transaction__deserialize1__excessive_version__nullptr)
{
    BOOST_REQUIRE(!transaction::deserialize(level::maximum_protocol + 1u, genesis_tx, true));
}

BOOST_AUTO_TEST_CASE(transaction__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("0100000001");
    BOOST_REQUIRE(!transaction::deserialize(level::minimum_protocol, data, true));
}

BOOST_AUTO_TEST_CASE(transaction__serialize1__witness_coinbase_840000__round_trips)
{
    const auto message = transaction::deserialize(level::minimum_protocol, coinbase_840000, true);
    BOOST_REQUIRE(message);
    system::data_chunk data(message->size(level::minimum_protocol, true));
    BOOST_REQUIRE(message->serialize(level::minimum_protocol, data, true));
    BOOST_REQUIRE_EQUAL(data, coinbase_840000);
}

BOOST_AUTO_TEST_CASE(transaction__serialize1__witness_coinbase_840000_without_witness__nominal_hash)
{
    const auto message = transaction::deserialize(level::minimum_protocol, coinbase_840000, true);
    BOOST_REQUIRE(message);
    system::data_chunk data(message->size(level::minimum_protocol, false));
    BOOST_REQUIRE(message->serialize(level::minimum_protocol, data, false));
    BOOST_REQUIRE_EQUAL(system::bitcoin_hash(data), coinbase_840000_txid);
}

BOOST_AUTO_TEST_CASE(transaction__serialize1__default__empty)
{
    system::data_chunk data{};
    BOOST_REQUIRE(transaction{}.serialize(level::minimum_protocol, data, true));
    BOOST_REQUIRE(data.empty());
}

BOOST_AUTO_TEST_SUITE_END()
