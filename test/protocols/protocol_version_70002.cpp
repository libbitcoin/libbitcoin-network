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

struct protocol_version_70002_setup_fixture
  : peer_net_setup_fixture<>
{
    protocol_version_70002_setup_fixture()
    {
        settings_.protocol_maximum = messages::peer::level::bip61;
        settings_.enable_reject = true;
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_version_70002_tests, protocol_version_70002_setup_fixture)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(protocol_version_70002__handshake__bip61_maximum__bip61_node_version)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip61));
    BOOST_REQUIRE_EQUAL(node_version->value, level::bip61);
    BOOST_REQUIRE(!node_version->relay);
}

BOOST_AUTO_TEST_CASE(protocol_version_70002__handshake__relay_enabled__relay_node_version)
{
    settings_.enable_relay = true;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip61));
    BOOST_REQUIRE(node_version->relay);
}

BOOST_AUTO_TEST_CASE(protocol_version_70002__receive_reject__during_handshake__handshake)
{
    BOOST_REQUIRE(open());
    send_version(level::bip61, service::node_none, network::unix_time());
    send(reject{ version::command, reject::reason_code::obsolete, "obsolete", {} }, level::bip61);
    BOOST_REQUIRE(receive_handshake(level::bip61));
    send(version_acknowledge{}, level::bip61);

    send(ping{ 42 }, level::bip61);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip61)->nonce, 42_u64);
}

BOOST_AUTO_TEST_CASE(protocol_version_70002__receive_version__duplicate_after_handshake__reject_duplicate)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip61));
    send_version(level::bip61, service::node_none, network::unix_time());

    const auto message = receive<reject>(level::bip61);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->message, version::command);
    BOOST_REQUIRE(message->code == reject::reason_code::duplicate);
}

BOOST_AUTO_TEST_CASE(protocol_version_70002__receive_version__below_minimum__dropped)
{
    settings_.protocol_minimum = level::bip61;
    BOOST_REQUIRE(open());
    send_version(level::bip37, service::node_none, network::unix_time());
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_70002__receive_version__invalid_services__dropped)
{
    BOOST_REQUIRE(open());
    send_version(level::bip61, settings_.invalid_services, network::unix_time());
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_70002__receive_version__timestamp_skew__dropped)
{
    BOOST_REQUIRE(open());
    send_version(level::bip61, service::node_none, network::unix_time() - (settings_.maximum_skew_minutes + 10u) * 60u);
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_70002__shake__no_peer_version__timeout_dropped)
{
    settings_.handshake_timeout_seconds = 1;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_SUITE_END()
