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

BOOST_AUTO_TEST_SUITE(p2p_client_filter_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(client_filter__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(client_filter::command, "cfilter");
    constexpr auto index = messages::peer::registry::index_of<client_filter>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), client_filter::command);
    BOOST_REQUIRE_EQUAL(client_filter::version_minimum, level::bip157);
    BOOST_REQUIRE_EQUAL(client_filter::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(client_filter__size__default__expected)
{
    constexpr auto expected = sizeof(uint8_t)
        + system::hash_size
        + variable_size(zero);

    BOOST_REQUIRE_EQUAL(client_filter{}.size(level::canonical), expected);
}

// bitcoin/src/test/data/blockfilters.json (testnet genesis basic filter)
static const auto genesis_hash = system::base16_hash("000000000933ea01ad0ee984209779baaec3ced90fa3f408719526f8d77f4943");
static const auto payload = system::base16_chunk("00" "43497fd7f826957108f4a30fd9cec3aeba79972084e90ead01ea330900000000" "04" "019dfca8");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(client_filter__size__genesis__expected)
{
    const client_filter message{ client_filter::type_id::neutrino, genesis_hash, system::base16_chunk("019dfca8") };
    BOOST_REQUIRE_EQUAL(message.size(level::bip157), payload.size());
}

BOOST_AUTO_TEST_CASE(client_filter__deserialize1__genesis__expected)
{
    const auto message = client_filter::deserialize(level::bip157, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->filter_type, client_filter::type_id::neutrino);
    BOOST_REQUIRE_EQUAL(message->block_hash, genesis_hash);
    BOOST_REQUIRE_EQUAL(message->filter, system::base16_chunk("019dfca8"));
}

BOOST_AUTO_TEST_CASE(client_filter__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!client_filter::deserialize(level::bip152, payload));
}

BOOST_AUTO_TEST_CASE(client_filter__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!client_filter::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(client_filter__deserialize1__underflow__nullptr)
{
    const system::data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BOOST_REQUIRE(!client_filter::deserialize(level::bip157, data));
}

BOOST_AUTO_TEST_CASE(client_filter__deserialize2__genesis__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = client_filter::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.block_hash, genesis_hash);
}

BOOST_AUTO_TEST_CASE(client_filter__serialize1__genesis__expected)
{
    const client_filter message{ client_filter::type_id::neutrino, genesis_hash, system::base16_chunk("019dfca8") };
    system::data_chunk data(message.size(level::bip157));
    BOOST_REQUIRE(message.serialize(level::bip157, data));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_SUITE_END()
