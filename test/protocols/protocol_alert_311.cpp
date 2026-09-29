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

struct protocol_alert_311_setup_fixture
  : peer_net_setup_fixture<>
{
    protocol_alert_311_setup_fixture()
    {
        settings_.enable_alert = true;
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_alert_311_tests, protocol_alert_311_setup_fixture)

using namespace network::messages::peer;

static const alert message
{
    alert_item
    {
        1,
        9223372036854775807,
        9223372036854775807,
        2147483647,
        2147483646,
        {},
        0,
        2147483647,
        {},
        2147483647,
        "",
        "URGENT: Alert key compromised, upgrade required",
        ""
    },
    system::data_chunk{ 0x30, 0x45, 0x02, 0x21, 0x00 }
};

BOOST_AUTO_TEST_CASE(protocol_alert_311__receive_alert__bip31_peer__connected)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));
    send(message, level::bip31);

    send(ping{ 42 }, level::bip31);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip31)->nonce, 42_u64);
}

BOOST_AUTO_TEST_SUITE_END()
