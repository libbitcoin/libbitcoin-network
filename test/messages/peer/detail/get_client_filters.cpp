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

BOOST_AUTO_TEST_SUITE(p2p_get_client_filters_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(get_client_filters__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(get_client_filters::command, "getcfilters");
    constexpr auto index = messages::peer::registry::index_of<get_client_filters>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), get_client_filters::command);
    BOOST_REQUIRE_EQUAL(get_client_filters::version_minimum, level::bip157);
    BOOST_REQUIRE_EQUAL(get_client_filters::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(get_client_filters__size__always__expected)
{
    constexpr auto expected = sizeof(uint8_t)
        + sizeof(uint32_t)
        + system::hash_size;

    BOOST_REQUIRE_EQUAL(get_client_filters::size(level::canonical), expected);
}

static const auto stop_hash = system::base16_hash("000000006c02c8ea6e4ff69651f7fcde348fb9d557a06e6957b65552002a7820");
static const auto payload = system::base16_chunk("00" "02000000" "20782a005255b657696ea057d5b98f34defcf75196f64f6eeac8026c00000000");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(get_client_filters__deserialize1__valid__expected)
{
    const auto message = get_client_filters::deserialize(level::bip157, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->filter_type, client_filter::type_id::neutrino);
    BOOST_REQUIRE_EQUAL(message->start_height, 2u);
    BOOST_REQUIRE_EQUAL(message->stop_hash, stop_hash);
}

BOOST_AUTO_TEST_CASE(get_client_filters__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!get_client_filters::deserialize(level::bip152, payload));
}

BOOST_AUTO_TEST_CASE(get_client_filters__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!get_client_filters::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(get_client_filters__deserialize1__underflow__nullptr)
{
    const system::data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BOOST_REQUIRE(!get_client_filters::deserialize(level::bip157, data));
}

BOOST_AUTO_TEST_CASE(get_client_filters__deserialize2__valid__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = get_client_filters::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.start_height, 2u);
}

BOOST_AUTO_TEST_CASE(get_client_filters__serialize1__valid__expected)
{
    const get_client_filters message{ client_filter::type_id::neutrino, 2, stop_hash };
    system::data_chunk data(get_client_filters::size(level::bip157));
    BOOST_REQUIRE(message.serialize(level::bip157, data));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_SUITE_END()
