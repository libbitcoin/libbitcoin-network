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

struct protocol_reject_70002_setup_fixture
  : peer_net_setup_fixture<>
{
    protocol_reject_70002_setup_fixture()
    {
        settings_.enable_reject = true;
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_reject_70002_tests, protocol_reject_70002_setup_fixture)

using namespace network::messages::peer;

static const system::hash_digest hash
{
    system::base16_hash("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f")
};

BOOST_AUTO_TEST_CASE(protocol_reject_70002__receive_reject__transaction__connected)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip61));
    send(reject{ transaction::command, reject::reason_code::insufficient_fee, "insufficient fee", hash }, level::bip61);

    send(ping{ 42 }, level::bip61);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip61)->nonce, 42_u64);
}

BOOST_AUTO_TEST_CASE(protocol_reject_70002__receive_reject__block__connected)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip61));
    send(reject{ block::command, reject::reason_code::invalid, "bad-blk", hash }, level::bip61);

    send(ping{ 42 }, level::bip61);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip61)->nonce, 42_u64);
}

BOOST_AUTO_TEST_CASE(protocol_reject_70002__receive_reject__unhashed_message__connected)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip155));
    send(reject{ get_data::command, reject::reason_code::malformed, "malformed", {} }, level::bip155);

    send(ping{ 42 }, level::bip155);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip155)->nonce, 42_u64);
}

BOOST_AUTO_TEST_SUITE_END()
