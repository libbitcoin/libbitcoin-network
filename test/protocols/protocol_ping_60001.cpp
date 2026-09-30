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

BOOST_FIXTURE_TEST_SUITE(protocol_ping_60001_tests, peer_net_setup_fixture<>)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(protocol_ping_60001__start__bip31_peer__ping_with_nonzero_nonce)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));

    const auto message = receive<ping>(level::bip31);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_NE(message->nonce, 0_u64);
}

BOOST_AUTO_TEST_CASE(protocol_ping_60001__receive_pong__matching_nonce__connected)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));

    const auto message = receive<ping>(level::bip31);
    BOOST_REQUIRE(message);
    send(pong{ message->nonce }, level::bip31);

    send(ping{ 42 }, level::bip31);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip31)->nonce, 42_u64);
}

BOOST_AUTO_TEST_CASE(protocol_ping_60001__handle_timer__zero_heartbeat_no_pong__dropped)
{
    settings_.channel_heartbeat_minutes = 0;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_SUITE_END()
