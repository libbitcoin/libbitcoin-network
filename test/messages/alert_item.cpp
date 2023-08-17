/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(alert_item_tests)

using namespace bc::network::messages;

const system::ec_uncompressed public_key
{
    {
        0x04, 0xfc, 0x97, 0x02, 0x84, 0x78, 0x40, 0xaa, 0xf1, 0x95, 0xde,
        0x84, 0x42, 0xeb, 0xec, 0xed, 0xf5, 0xb0, 0x95, 0xcd, 0xbb, 0x9b,
        0xc7, 0x16, 0xbd, 0xa9, 0x11, 0x09, 0x71, 0xb2, 0x8a, 0x49, 0xe0,
        0xea, 0xd8, 0x56, 0x4f, 0xf0, 0xdb, 0x22, 0x20, 0x9e, 0x03, 0x74,
        0x78, 0x2c, 0x09, 0x3b, 0xb8, 0x99, 0x69, 0x2d, 0x52, 0x4e, 0x9d,
        0x6a, 0x69, 0x56, 0xe7, 0xc5, 0xec, 0xbc, 0xd6, 0x82, 0x84
    }
};

BOOST_AUTO_TEST_CASE(alert_item__satoshi_public_key__always__expected)
{
    BOOST_REQUIRE_EQUAL(alert_item::satoshi_public_key, public_key);
}

BOOST_AUTO_TEST_CASE(alert_item__size__default__expected)
{
    constexpr auto expected = sizeof(uint32_t)
        + sizeof(uint64_t)
        + sizeof(uint64_t)
        + sizeof(uint32_t)
        + sizeof(uint32_t)
        + variable_size(zero)
        + sizeof(uint32_t)
        + sizeof(uint32_t)
        + variable_size(zero)
        + sizeof(uint32_t)
        + variable_size(zero)
        + variable_size(zero)
        + variable_size(zero);

    BOOST_REQUIRE_EQUAL(alert_item{}.size(level::canonical), expected);
}

BOOST_AUTO_TEST_CASE(alert_item__deserialize__bitcoin_wiki_sample__expected)
{
    // en.bitcoin.it/wiki/Protocol_documentation#alert
    constexpr auto payload = system::base16_array(
        "010000003766404f00000000b305434f00000000f2030000f10300000010270000"
        "48ee00000064000000004653656520626974636f696e2e6f72672f66656232302069"
        "6620796f7520686176652074726f75626c6520636f6e6e656374696e672061667465"
        "7220323020466562727561727900");
    constexpr auto expected_status_bar =
        "See bitcoin.org/feb20 if you have trouble connecting after 20 February";

    system::read::bytes::copy source(payload);
    const auto message = alert_item::deserialize(0, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.version, 1u);
    BOOST_REQUIRE_EQUAL(message.relay_until, 1329620535u);
    BOOST_REQUIRE_EQUAL(message.expiration, 1329792435u);
    BOOST_REQUIRE_EQUAL(message.id, 1010u);
    BOOST_REQUIRE_EQUAL(message.cancel, 1009u);
    BOOST_REQUIRE(message.cancels.empty());
    BOOST_REQUIRE_EQUAL(message.min_version, 10000u);
    BOOST_REQUIRE_EQUAL(message.max_version, 61000u);
    BOOST_REQUIRE(message.sub_versions.empty());
    BOOST_REQUIRE_EQUAL(message.priority, 100u);
    BOOST_REQUIRE(message.comment.empty());
    BOOST_REQUIRE_EQUAL(message.status_bar, expected_status_bar);
    BOOST_REQUIRE(message.reserved.empty());
}

BOOST_AUTO_TEST_SUITE_END()
