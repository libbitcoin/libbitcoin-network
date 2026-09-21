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
#include "peer_setup_fixture.hpp"

BOOST_FIXTURE_TEST_SUITE(functional_peer_tests, peer_setup_fixture)

using namespace messages::peer;

BOOST_AUTO_TEST_CASE(functional_peer__handshake__default__expected_version)
{
    BOOST_REQUIRE(handshake());
    BOOST_REQUIRE_EQUAL(node_version->value, settings_.protocol_maximum);
    BOOST_REQUIRE_EQUAL(node_version->services, service::node_none);
    BOOST_REQUIRE_EQUAL(node_version->user_agent, settings_.user_agent);
}

BOOST_AUTO_TEST_CASE(functional_peer__ping__nonce__pong_echo)
{
    BOOST_REQUIRE(handshake());

    constexpr uint64_t expected = 42;
    send(ping{ expected }, node_version->value);

    const auto payload = receive(pong::command);
    const auto message = pong::deserialize(node_version->value, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->nonce, expected);
}

BOOST_AUTO_TEST_CASE(functional_peer__ping__pipelined__ponged_in_order)
{
    BOOST_REQUIRE(handshake());

    send(ping{ 1 }, node_version->value);
    send(ping{ 2 }, node_version->value);
    send(ping{ 3 }, node_version->value);

    BOOST_REQUIRE_EQUAL(pong::deserialize(node_version->value, receive(pong::command))->nonce, 1_u64);
    BOOST_REQUIRE_EQUAL(pong::deserialize(node_version->value, receive(pong::command))->nonce, 2_u64);
    BOOST_REQUIRE_EQUAL(pong::deserialize(node_version->value, receive(pong::command))->nonce, 3_u64);
}

BOOST_AUTO_TEST_SUITE_END()
