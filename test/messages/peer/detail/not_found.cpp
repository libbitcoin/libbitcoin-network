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

BOOST_AUTO_TEST_SUITE(p2p_not_found_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(not_found__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(not_found::command, "notfound");
    constexpr auto index = messages::peer::registry::index_of<not_found>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), not_found::command);
    BOOST_REQUIRE_EQUAL(not_found::version_minimum, level::bip37);
    BOOST_REQUIRE_EQUAL(not_found::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(not_found__size__default__expected)
{
    constexpr auto expected = variable_size(zero);

    // passed to base class.
    BOOST_REQUIRE_EQUAL(not_found{}.size(level::canonical), expected);
}

static const auto genesis_tx_hash = system::base16_hash("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
static const auto genesis_tx_payload = system::base16_chunk("01010000003ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a");

BOOST_AUTO_TEST_CASE(not_found__deserialize1__genesis_tx__expected)
{
    const auto message = not_found::deserialize(level::bip37, genesis_tx_payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->items.size(), one);
    BOOST_REQUIRE(message->items.front().type == inventory_item::type_id::transaction);
    BOOST_REQUIRE_EQUAL(message->items.front().hash, genesis_tx_hash);
}

BOOST_AUTO_TEST_CASE(not_found__deserialize1__empty__expected)
{
    const auto message = not_found::deserialize(level::bip37, system::base16_chunk("00"));
    BOOST_REQUIRE(message);
    BOOST_REQUIRE(message->items.empty());
}

BOOST_AUTO_TEST_CASE(not_found__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!not_found::deserialize(level::bip37 - 1u, genesis_tx_payload));
}

BOOST_AUTO_TEST_CASE(not_found__deserialize1__excessive_version__nullptr)
{
    BOOST_REQUIRE(!not_found::deserialize(level::maximum_protocol + 1u, genesis_tx_payload));
}

BOOST_AUTO_TEST_CASE(not_found__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("0101000000");
    BOOST_REQUIRE(!not_found::deserialize(level::bip37, data));
}

BOOST_AUTO_TEST_CASE(not_found__deserialize1__excess_count__nullptr)
{
    const auto data = system::base16_chunk("fe51c30000");
    BOOST_REQUIRE(!not_found::deserialize(level::bip37, data));
}

BOOST_AUTO_TEST_CASE(not_found__serialize1__genesis_tx__expected)
{
    const not_found message{ { { inventory_item::type_id::transaction, genesis_tx_hash } } };
    BOOST_REQUIRE_EQUAL(message.size(level::bip37), genesis_tx_payload.size());
    system::data_chunk data(message.size(level::bip37));
    BOOST_REQUIRE(message.serialize(level::bip37, data));
    BOOST_REQUIRE_EQUAL(data, genesis_tx_payload);
}

BOOST_AUTO_TEST_SUITE_END()
