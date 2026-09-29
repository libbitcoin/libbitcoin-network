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
#include "../functional/peer_setup_fixture.hpp"

struct protocol_version_70001_setup_fixture
  : peer_net_setup_fixture<>
{
    protocol_version_70001_setup_fixture()
    {
        settings_.protocol_maximum = messages::peer::level::bip37;
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_version_70001_tests, protocol_version_70001_setup_fixture)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(protocol_version_70001__handshake__bip37_maximum__bip37_node_version_no_relay)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip37));
    BOOST_REQUIRE_EQUAL(node_version->value, level::bip37);
    BOOST_REQUIRE(!node_version->relay);
}

BOOST_AUTO_TEST_CASE(protocol_version_70001__handshake__relay_enabled__relay_node_version)
{
    settings_.enable_relay = true;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip37));
    BOOST_REQUIRE(node_version->relay);
}

BOOST_AUTO_TEST_CASE(protocol_version_70001__handshake__reject_disabled_bip61_maximum__bip61_node_version)
{
    settings_.protocol_maximum = level::bip61;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip61));
    BOOST_REQUIRE_EQUAL(node_version->value, level::bip61);

    send(ping{ 42 }, level::bip61);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip61)->nonce, 42_u64);
}

BOOST_AUTO_TEST_SUITE_END()
