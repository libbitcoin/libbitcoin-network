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

BOOST_FIXTURE_TEST_SUITE(protocol_ping_106_tests, peer_net_setup_fixture<>)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(protocol_ping_106__start__address_timestamp_peer__ping_without_nonce)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::address_timestamp));
    BOOST_REQUIRE(receive(ping::command).empty());
}

BOOST_AUTO_TEST_CASE(protocol_ping_106__handle_timer__zero_heartbeat__pings_repeated)
{
    settings_.channel_heartbeat_minutes = 0;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::address_timestamp));
    BOOST_REQUIRE(receive(ping::command).empty());
    BOOST_REQUIRE(receive(ping::command).empty());
    BOOST_REQUIRE(receive(ping::command).empty());
}

BOOST_AUTO_TEST_CASE(protocol_ping_106__receive_ping__address_timestamp_peer__no_pong)
{
    settings_.channel_heartbeat_minutes = 0;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::address_timestamp));
    send(ping{}, level::address_timestamp);
    BOOST_REQUIRE_EQUAL(receive().first, ping::command);
    BOOST_REQUIRE_EQUAL(receive().first, ping::command);
    BOOST_REQUIRE_EQUAL(receive().first, ping::command);
}

BOOST_AUTO_TEST_SUITE_END()
