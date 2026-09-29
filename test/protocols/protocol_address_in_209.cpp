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

struct protocol_address_in_209_setup_fixture
  : peer_net_setup_fixture<>
{
    protocol_address_in_209_setup_fixture()
    {
        settings_.enable_address = true;
        settings_.outbound.host_pool_capacity = 100;
    }

    // Complete prior node work on the channel and the network strand.
    void flush(uint32_t version)
    {
        send(messages::peer::ping{ 42 }, version);
        BOOST_REQUIRE_EQUAL(receive<messages::peer::pong>(version)->nonce, 42_u64);
        synchronize();
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_address_in_209_tests, protocol_address_in_209_setup_fixture)

using namespace network::messages::peer;

static const address_item host1 = network::config::address{ "1.2.3.4:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host2 = network::config::address{ "5.6.7.8:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host3 = network::config::address{ "9.10.11.12:8333" }.to_address_item(1700000000, service::node_network);
static const address_item seed = network::config::address{ PEER_LISTEN_ENDPOINT }.to_address_item(1700000000, service::node_network);

BOOST_AUTO_TEST_CASE(protocol_address_in_209__start__outbound__get_address)
{
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip31));
    BOOST_REQUIRE(receive(get_address::command).empty());
}

BOOST_AUTO_TEST_CASE(protocol_address_in_209__receive_address__outbound__saved)
{
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { host1, host2, host3 } }, level::bip31);
    flush(level::bip31);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 3u);
}

BOOST_AUTO_TEST_CASE(protocol_address_in_209__receive_address__multiple_messages__all_saved)
{
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { host1 } }, level::bip31);
    send(address{ { host2, host3 } }, level::bip31);
    flush(level::bip31);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 3u);
}

BOOST_AUTO_TEST_CASE(protocol_address_in_209__receive_address__unsupported_services__excluded)
{
    auto unsupported = host2;
    unsupported.services = settings_.invalid_services;
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { host1, unsupported } }, level::bip31);
    flush(level::bip31);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 1u);
}

BOOST_AUTO_TEST_CASE(protocol_address_in_209__receive_address__peer_own_address__not_saved)
{
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { seed } }, level::bip31);
    flush(level::bip31);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 0u);
}

BOOST_AUTO_TEST_CASE(protocol_address_in_209__receive_address__pool_full__not_saved)
{
    settings_.outbound.host_pool_capacity = 1;
    write_hosts({ host1 });
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { host2, host3 } }, level::bip31);
    flush(level::bip31);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 1u);
}

BOOST_AUTO_TEST_SUITE_END()
