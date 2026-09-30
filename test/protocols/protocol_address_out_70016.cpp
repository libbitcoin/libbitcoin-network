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

struct protocol_address_out_70016_setup_fixture
  : peer_net_setup_fixture<>
{
    protocol_address_out_70016_setup_fixture()
    {
        settings_.enable_address = true;
        settings_.outbound.host_pool_capacity = 100;
        settings_.address_lower = 1;
        settings_.address_upper = 1;
    }

    // Handshake as a peer that requests address v2 (bip155).
    bool handshake_v2()
    {
        using namespace messages::peer;
        send_version(level::bip155, service::node_none, network::unix_time());
        send(send_address_v2{}, level::bip155);
        if (!receive_handshake(level::bip155))
            return false;

        send(version_acknowledge{}, level::bip155);
        return true;
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_address_out_70016_tests, protocol_address_out_70016_setup_fixture)

using namespace network::messages::peer;

static const address_item host1 = network::config::address{ "1.2.3.4:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host2 = network::config::address{ "5.6.7.8:8333" }.to_address_item(1700000000, service::node_network);
static const network::config::address self4{ "9.9.9.9:8333" };

BOOST_AUTO_TEST_CASE(protocol_address_out_70016__start__selfs__advertised_address_v2)
{
    settings_.inbound.selfs.push_back(self4);
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake_v2());

    const auto message = receive<address_v2>(level::bip155);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->addresses.size(), 1u);
    BOOST_REQUIRE(self4 == message->addresses.front());
}

BOOST_AUTO_TEST_CASE(protocol_address_out_70016__receive_get_address__pooled__address_v2)
{
    write_hosts({ host1, host2 });
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake_v2());
    send(get_address{}, level::bip155);

    const auto message = receive<address_v2>(level::bip155);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->addresses.size(), 2u);
}

BOOST_AUTO_TEST_CASE(protocol_address_out_70016__receive_get_address__not_requested_v2__address)
{
    write_hosts({ host1, host2 });
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip155));
    send(get_address{}, level::bip155);

    const auto message = receive<address>(level::bip155);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->addresses.size(), 2u);
}

BOOST_AUTO_TEST_CASE(protocol_address_out_70016__handle_broadcast_address__advertisement_from_outbound__relayed_address_v2)
{
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip155));

    remote_peer inbound{ settings_.identifier };
    inbound.connect(settings_.inbound.binds.back().to_endpoint());
    inbound.send_version(level::bip155, service::node_none, network::unix_time());
    inbound.send(send_address_v2{}, level::bip155);
    BOOST_REQUIRE(inbound.receive_handshake(level::bip155));
    inbound.send(version_acknowledge{}, level::bip155);
    inbound.send(get_address{}, level::bip155);
    inbound.send(ping{ 42 }, level::bip155);
    BOOST_REQUIRE_EQUAL(inbound.receive<pong>(level::bip155)->nonce, 42_u64);

    send(address_v2{ { host1 } }, level::bip155);

    const auto message = inbound.receive<address_v2>(level::bip155);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->addresses.size(), 1u);
    BOOST_REQUIRE(message->addresses.front() == host1);
}

BOOST_AUTO_TEST_SUITE_END()
