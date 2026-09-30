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

BOOST_AUTO_TEST_SUITE(p2p_client_filter_checkpoint_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(client_filter_checkpoint__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(client_filter_checkpoint::command, "cfcheckpt");
    constexpr auto index = messages::peer::registry::index_of<client_filter_checkpoint>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), client_filter_checkpoint::command);
    BOOST_REQUIRE_EQUAL(client_filter_checkpoint::version_minimum, level::bip157);
    BOOST_REQUIRE_EQUAL(client_filter_checkpoint::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(client_filter_checkpoint__size__default__expected)
{
    constexpr auto expected = sizeof(uint8_t)
        + system::hash_size
        + variable_size(zero);

    BOOST_REQUIRE_EQUAL(client_filter_checkpoint{}.size(level::canonical), expected);
}

// bitcoin/src/test/data/blockfilters.json (testnet basic filter headers)
static const auto stop_hash = system::base16_hash("000000006c02c8ea6e4ff69651f7fcde348fb9d557a06e6957b65552002a7820");
static const auto header0 = system::base16_hash("21584579b7eb08997773e5aeff3a7f932700042d0ed2a6129012b7d7ae81b750");
static const auto header2 = system::base16_hash("186afd11ef2b5e7e3504f2e8cbf8df28a1fd251fe53d60dff8b1467d1b386cf0");
static const auto payload = system::base16_chunk(
    "00"
    "20782a005255b657696ea057d5b98f34defcf75196f64f6eeac8026c00000000"
    "02"
    "50b781aed7b7129012a6d20e2d040027937f3affaee573779908ebb779455821"
    "f06c381b7d46b1f8df603de51f25fda128dff8cbe8f204357e5e2bef11fd6a18");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(client_filter_checkpoint__size__two__expected)
{
    const client_filter_checkpoint message{ client_filter::type_id::neutrino, stop_hash, { header0, header2 } };
    BOOST_REQUIRE_EQUAL(message.size(level::bip157), payload.size());
}

BOOST_AUTO_TEST_CASE(client_filter_checkpoint__deserialize1__two__expected)
{
    const auto message = client_filter_checkpoint::deserialize(level::bip157, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->filter_type, client_filter::type_id::neutrino);
    BOOST_REQUIRE_EQUAL(message->stop_hash, stop_hash);
    BOOST_REQUIRE_EQUAL(message->filter_headers.size(), two);
    BOOST_REQUIRE_EQUAL(message->filter_headers.front(), header0);
    BOOST_REQUIRE_EQUAL(message->filter_headers.back(), header2);
}

BOOST_AUTO_TEST_CASE(client_filter_checkpoint__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!client_filter_checkpoint::deserialize(level::bip152, payload));
}

BOOST_AUTO_TEST_CASE(client_filter_checkpoint__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!client_filter_checkpoint::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(client_filter_checkpoint__deserialize1__underflow__nullptr)
{
    const system::data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BOOST_REQUIRE(!client_filter_checkpoint::deserialize(level::bip157, data));
}

BOOST_AUTO_TEST_CASE(client_filter_checkpoint__deserialize2__two__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = client_filter_checkpoint::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.filter_headers.size(), two);
}

BOOST_AUTO_TEST_CASE(client_filter_checkpoint__serialize1__two__expected)
{
    const client_filter_checkpoint message{ client_filter::type_id::neutrino, stop_hash, { header0, header2 } };
    system::data_chunk data(message.size(level::bip157));
    BOOST_REQUIRE(message.serialize(level::bip157, data));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_SUITE_END()
