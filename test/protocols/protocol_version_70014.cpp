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

struct protocol_version_70014_setup_fixture
  : peer_net_setup_fixture<>
{
    protocol_version_70014_setup_fixture()
    {
        settings_.enable_address_v2 = false;
        settings_.enable_compact = true;
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_version_70014_tests, protocol_version_70014_setup_fixture)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(protocol_version_70014__handshake__bip152_peer__send_compact_low_bandwidth_version_2)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip152));
    BOOST_REQUIRE_EQUAL(node_version->value, level::maximum_protocol);

    const auto message = receive<send_compact>(level::bip152);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE(!message->high_bandwidth);
    BOOST_REQUIRE_EQUAL(message->compact_version, send_compact::compact_version_2);
}

BOOST_AUTO_TEST_CASE(protocol_version_70014__handshake__bip61_peer__no_send_compact)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip61));
    BOOST_REQUIRE_EQUAL(receive().first, ping::command);
}

BOOST_AUTO_TEST_SUITE_END()
