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

struct protocol_address_in_70016_setup_fixture
  : peer_net_setup_fixture<>
{
    protocol_address_in_70016_setup_fixture()
    {
        settings_.enable_address = true;
        settings_.outbound.host_pool_capacity = 100;
    }

    // Complete prior node work on the channel and the network strand.
    void flush()
    {
        send(messages::peer::ping{ 42 }, messages::peer::level::bip155);
        BOOST_REQUIRE_EQUAL(receive<messages::peer::pong>(messages::peer::level::bip155)->nonce, 42_u64);
        synchronize();
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_address_in_70016_tests, protocol_address_in_70016_setup_fixture)

using namespace network::messages::peer;

static const address_item host1 = network::config::address{ "1.2.3.4:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host2 = network::config::address{ "5.6.7.8:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host3 = network::config::address{ "9.10.11.12:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host6 = network::config::address{ "[2001:db8::1]:8333" }.to_address_item(1700000000, service::node_network);

BOOST_AUTO_TEST_CASE(protocol_address_in_70016__start__outbound__get_address)
{
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip155));
    BOOST_REQUIRE(receive(get_address::command).empty());
}

BOOST_AUTO_TEST_CASE(protocol_address_in_70016__receive_address_v2__outbound__saved)
{
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip155));
    send(address_v2{ { host1, host2, host3 } }, level::bip155);
    flush();
    BOOST_REQUIRE_EQUAL(net_->address_count(), 3u);
}

BOOST_AUTO_TEST_CASE(protocol_address_in_70016__receive_address_v2__ungossiped_network__excluded)
{
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip155));
    send(address_v2{ { host1, host6 } }, level::bip155);
    flush();
    BOOST_REQUIRE_EQUAL(net_->address_count(), 1u);
}

BOOST_AUTO_TEST_CASE(protocol_address_in_70016__receive_address__outbound__saved)
{
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip155));
    send(address{ { host1, host2 } }, level::bip155);
    flush();
    BOOST_REQUIRE_EQUAL(net_->address_count(), 2u);
}

BOOST_AUTO_TEST_SUITE_END()
