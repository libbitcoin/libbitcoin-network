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

BOOST_AUTO_TEST_SUITE(alert_tests)

using namespace bc::network::messages;

BOOST_AUTO_TEST_CASE(alert__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(alert::command, "alert");
    BOOST_REQUIRE(alert::id == identifier::alert);
    BOOST_REQUIRE_EQUAL(alert::version_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(alert::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(alert__size__default__expected)
{
    const auto item = alert_item{}.size(level::canonical);
    const auto expected = variable_size(item) + item + variable_size(zero);
    BOOST_REQUIRE_EQUAL(expected, alert{}.size(level::canonical));
}

BOOST_AUTO_TEST_CASE(alert__deserialize__bitcoin_wiki_sample__expected)
{
    // en.bitcoin.it/wiki/Protocol_documentation#alert
    constexpr auto payload = system::base16_array(
        "73"
        "010000003766404f00000000b305434f00000000f2030000f10300000010270000"
        "48ee00000064000000004653656520626974636f696e2e6f72672f66656232302069"
        "6620796f7520686176652074726f75626c6520636f6e6e656374696e672061667465"
        "7220323020466562727561727900"
        "47"
        "30450221008389df45f0703f39ec8c1cc42c13"
        "810ffcae14995bb648340219e353b63b53eb022009ec65e1c1aaeec1fd334c6b684b"
        "de2b3f573060d5b70c3a46723326e4e8a4f1");
    constexpr auto expected_status_bar =
        "See bitcoin.org/feb20 if you have trouble connecting after 20 February";
    const auto expected_signature = system::base16_chunk(
        "30450221008389df45f0703f39ec8c1cc42c13810ffcae14995bb648340219e353b63b53"
        "eb022009ec65e1c1aaeec1fd334c6b684bde2b3f573060d5b70c3a46723326e4e8a4f1");

    system::read::bytes::copy source(payload);
    const auto message = alert::deserialize(level::minimum_protocol, source);
    BOOST_REQUIRE(source);

    BOOST_REQUIRE_EQUAL(message.payload.version, 1u);
    BOOST_REQUIRE_EQUAL(message.payload.relay_until, 1329620535u);
    BOOST_REQUIRE_EQUAL(message.payload.expiration, 1329792435u);
    BOOST_REQUIRE_EQUAL(message.payload.id, 1010u);
    BOOST_REQUIRE_EQUAL(message.payload.cancel, 1009u);
    BOOST_REQUIRE(message.payload.cancels.empty());
    BOOST_REQUIRE_EQUAL(message.payload.min_version, 10000u);
    BOOST_REQUIRE_EQUAL(message.payload.max_version, 61000u);
    BOOST_REQUIRE(message.payload.sub_versions.empty());
    BOOST_REQUIRE_EQUAL(message.payload.priority, 100u);
    BOOST_REQUIRE(message.payload.comment.empty());
    BOOST_REQUIRE_EQUAL(message.payload.status_bar, expected_status_bar);
    BOOST_REQUIRE(message.payload.reserved.empty());

    BOOST_REQUIRE_EQUAL(message.signature, expected_signature);
}

BOOST_AUTO_TEST_SUITE_END()
