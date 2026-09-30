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

struct protocol_seed_209_setup_fixture
  : peer_net_setup_fixture<>
{
    protocol_seed_209_setup_fixture()
    {
        settings_.outbound.connections = 1;
        settings_.outbound.host_pool_capacity = 100;
        settings_.outbound.seeds.emplace_back(PEER_LISTEN_ENDPOINT);
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_seed_209_tests, protocol_seed_209_setup_fixture)

using namespace network::messages::peer;

static const address_item host1 = network::config::address{ "1.2.3.4:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host2 = network::config::address{ "5.6.7.8:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host3 = network::config::address{ "9.10.11.12:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host4 = network::config::address{ "13.14.15.16:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host5 = network::config::address{ "17.18.19.20:8333" }.to_address_item(1700000000, service::node_network);
static const address_item host6 = network::config::address{ "21.22.23.24:8333" }.to_address_item(1700000000, service::node_network);
static const address_item seed = network::config::address{ PEER_LISTEN_ENDPOINT }.to_address_item(1700000000, service::node_network);
static const network::config::address self4{ "9.9.9.9:8333" };

BOOST_AUTO_TEST_CASE(protocol_seed_209__start__seed_peer__get_address)
{
    auto started = starting();
    accept();
    BOOST_REQUIRE(handshake(level::bip31));
    BOOST_REQUIRE(receive(get_address::command).empty());
    send(address{ { host1, host2, host3, host4, host5 } }, level::bip31);
    BOOST_REQUIRE(dropped());
    BOOST_REQUIRE_EQUAL(started.get(), error::success);
}

BOOST_AUTO_TEST_CASE(protocol_seed_209__receive_address__sufficient__seeded)
{
    auto started = starting();
    accept();
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { host1, host2, host3, host4, host5 } }, level::bip31);
    BOOST_REQUIRE(dropped());
    BOOST_REQUIRE_EQUAL(started.get(), error::success);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 5u);
}

BOOST_AUTO_TEST_CASE(protocol_seed_209__receive_address__insufficient__seeding_unsuccessful)
{
    auto started = starting();
    accept();
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { host1, host2 } }, level::bip31);
    BOOST_REQUIRE(dropped());
    BOOST_REQUIRE_EQUAL(started.get(), error::seeding_unsuccessful);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 2u);
}

BOOST_AUTO_TEST_CASE(protocol_seed_209__receive_address__singleton_then_sufficient__seeded)
{
    auto started = starting();
    accept();
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { host6 } }, level::bip31);
    send(address{ { host1, host2, host3, host4, host5 } }, level::bip31);
    BOOST_REQUIRE(dropped());
    BOOST_REQUIRE_EQUAL(started.get(), error::success);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 6u);
}

BOOST_AUTO_TEST_CASE(protocol_seed_209__receive_address__seed_own_address__not_saved)
{
    auto started = starting();
    accept();
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { seed } }, level::bip31);
    send(address{ { host1, host2, host3, host4, host5 } }, level::bip31);
    BOOST_REQUIRE(dropped());
    BOOST_REQUIRE_EQUAL(started.get(), error::success);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 5u);
}

BOOST_AUTO_TEST_CASE(protocol_seed_209__receive_address__all_excluded__seeding_unsuccessful)
{
    auto unsupported1 = host1;
    auto unsupported2 = host2;
    unsupported1.services = settings_.invalid_services;
    unsupported2.services = settings_.invalid_services;
    auto started = starting();
    accept();
    BOOST_REQUIRE(handshake(level::bip31));
    send(address{ { unsupported1, unsupported2 } }, level::bip31);
    BOOST_REQUIRE(dropped());
    BOOST_REQUIRE_EQUAL(started.get(), error::seeding_unsuccessful);
    BOOST_REQUIRE_EQUAL(net_->address_count(), 0u);
}

BOOST_AUTO_TEST_CASE(protocol_seed_209__receive_get_address__selfs__self_address)
{
    settings_.inbound.selfs.push_back(self4);
    auto started = starting();
    accept();
    BOOST_REQUIRE(handshake(level::bip31));
    send(get_address{}, level::bip31);

    const auto message = receive<address>(level::bip31);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->addresses.size(), 1u);
    BOOST_REQUIRE(self4 == message->addresses.front());

    send(address{ { host1, host2, host3, host4, host5 } }, level::bip31);
    BOOST_REQUIRE(dropped());
    BOOST_REQUIRE_EQUAL(started.get(), error::success);
}

BOOST_AUTO_TEST_CASE(protocol_seed_209__receive_get_address__no_selfs__seeded)
{
    auto started = starting();
    accept();
    BOOST_REQUIRE(handshake(level::bip31));
    send(get_address{}, level::bip31);
    send(address{ { host1, host2, host3, host4, host5 } }, level::bip31);
    BOOST_REQUIRE(dropped());
    BOOST_REQUIRE_EQUAL(started.get(), error::success);
}

BOOST_AUTO_TEST_CASE(protocol_seed_209__handle_timer__no_address__dropped)
{
    settings_.outbound.seeding_timeout_seconds = 1;
    auto started = starting();
    accept();
    BOOST_REQUIRE(handshake(level::bip31));
    BOOST_REQUIRE(dropped());
    BOOST_REQUIRE_EQUAL(started.get(), error::seeding_unsuccessful);
}

BOOST_AUTO_TEST_SUITE_END()
