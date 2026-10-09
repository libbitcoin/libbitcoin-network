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

BOOST_FIXTURE_TEST_SUITE(protocol_version_70016_tests, peer_net_setup_fixture<>)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(protocol_version_70016__handshake__address_gossip_v2__send_address_v2_before_acknowledge)
{
    settings_.enable_address = true;
    settings_.gossip_tor = true;
    BOOST_REQUIRE(open());

    send_version(level::bip155, service::node_none, network::unix_time());
    BOOST_REQUIRE_EQUAL(receive().first, version::command);
    BOOST_REQUIRE_EQUAL(receive().first, send_address_v2::command);
    BOOST_REQUIRE_EQUAL(receive().first, version_acknowledge::command);
}

BOOST_AUTO_TEST_CASE(protocol_version_70016__handshake__address_no_gossip_v2__no_send_address_v2)
{
    settings_.enable_address = true;
    BOOST_REQUIRE(open());

    send_version(level::bip155, service::node_none, network::unix_time());
    BOOST_REQUIRE_EQUAL(receive().first, version::command);
    BOOST_REQUIRE_EQUAL(receive().first, version_acknowledge::command);
}

BOOST_AUTO_TEST_CASE(protocol_version_70016__handshake__bip152_peer_address_gossip_v2__no_send_address_v2)
{
    settings_.enable_address = true;
    settings_.gossip_tor = true;
    BOOST_REQUIRE(open());

    send_version(level::bip152, service::node_none, network::unix_time());
    BOOST_REQUIRE_EQUAL(receive().first, version::command);
    BOOST_REQUIRE_EQUAL(receive().first, version_acknowledge::command);
}

BOOST_AUTO_TEST_CASE(protocol_version_70016__handshake__send_address_v2_and_witness_tx_id_relay_before_acknowledge__handshake)
{
    BOOST_REQUIRE(open());

    send_version(level::bip155, service::node_none, network::unix_time());
    send(send_address_v2{}, level::bip155);
    send(witness_tx_id_relay{}, level::bip155);
    BOOST_REQUIRE(receive_handshake(level::bip155));

    send(version_acknowledge{}, level::bip155);
    send(ping{ 42 }, level::bip155);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip155)->nonce, 42_u64);
}

BOOST_AUTO_TEST_CASE(protocol_version_70016__handshake__witness_tx__witness_tx_id_relay_before_acknowledge)
{
    settings_.enable_witness_tx = true;
    BOOST_REQUIRE(open());

    send_version(level::bip339, service::node_none, network::unix_time());
    BOOST_REQUIRE_EQUAL(receive().first, version::command);
    BOOST_REQUIRE_EQUAL(receive().first, witness_tx_id_relay::command);
    BOOST_REQUIRE_EQUAL(receive().first, version_acknowledge::command);
}

BOOST_AUTO_TEST_CASE(protocol_version_70016__handshake__witness_tx_address_v2_disabled__witness_tx_id_relay_before_acknowledge)
{
    settings_.enable_address = true;
    settings_.enable_address_v2 = false;
    settings_.enable_witness_tx = true;
    settings_.gossip_tor = true;
    BOOST_REQUIRE(open());

    send_version(level::bip339, service::node_none, network::unix_time());
    BOOST_REQUIRE_EQUAL(receive().first, version::command);
    BOOST_REQUIRE_EQUAL(receive().first, witness_tx_id_relay::command);
    BOOST_REQUIRE_EQUAL(receive().first, version_acknowledge::command);
}

BOOST_AUTO_TEST_CASE(protocol_version_70016__handshake__no_witness_tx__no_witness_tx_id_relay)
{
    BOOST_REQUIRE(open());

    send_version(level::bip339, service::node_none, network::unix_time());
    BOOST_REQUIRE_EQUAL(receive().first, version::command);
    BOOST_REQUIRE_EQUAL(receive().first, version_acknowledge::command);
}

BOOST_AUTO_TEST_CASE(protocol_version_70016__handshake__witness_tx_bip152_peer__no_witness_tx_id_relay)
{
    settings_.enable_witness_tx = true;
    BOOST_REQUIRE(open());

    send_version(level::bip152, service::node_none, network::unix_time());
    BOOST_REQUIRE_EQUAL(receive().first, version::command);
    BOOST_REQUIRE_EQUAL(receive().first, version_acknowledge::command);
}

BOOST_AUTO_TEST_CASE(protocol_version_70016__receive_witness_tx_id_relay__after_acknowledge__dropped)
{
    settings_.enable_witness_tx = true;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip339));

    send(witness_tx_id_relay{}, level::bip339);
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_70016__receive_version__reject_enabled_duplicate_after_handshake__reject_duplicate)
{
    settings_.enable_reject = true;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip155));

    send_version(level::bip155, service::node_none, network::unix_time());
    const auto message = receive<reject>(level::bip155);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->message, version::command);
    BOOST_REQUIRE(message->code == reject::reason_code::duplicate);
}

BOOST_AUTO_TEST_SUITE_END()
