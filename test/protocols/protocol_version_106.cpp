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

class network_required_session
  : public session_inbound
{
public:
    using session_inbound::session_inbound;

    uint64_t services_required() const NOEXCEPT override
    {
        return messages::peer::service::node_network;
    }
};

class network_required_net
  : public net
{
public:
    using net::net;

protected:
    session_inbound::ptr attach_inbound_session() NOEXCEPT override
    {
        return attach<network_required_session>(*this);
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_version_106_tests, peer_net_setup_fixture<>)

using namespace network::messages::peer;

BOOST_FIXTURE_TEST_CASE(protocol_version_106__receive_version__insufficient_services__dropped, peer_net_setup_fixture<network_required_net>)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    send_version(level::bip31, service::node_none, network::unix_time());
    BOOST_REQUIRE(dropped());
}

BOOST_FIXTURE_TEST_CASE(protocol_version_106__receive_version__sufficient_services__handshake, peer_net_setup_fixture<network_required_net>)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31, service::node_network));

    send(ping{ 42 }, level::bip31);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip31)->nonce, 42_u64);
}

BOOST_AUTO_TEST_CASE(protocol_version_106__handshake__bip31_maximum__bip31_node_version)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));
    BOOST_REQUIRE_EQUAL(node_version->value, level::bip31);
    BOOST_REQUIRE_EQUAL(node_version->services, service::node_none);
    BOOST_REQUIRE_EQUAL(node_version->user_agent, settings_.user_agent);
    BOOST_REQUIRE_EQUAL(node_version->address_receiver.port, port());
}

BOOST_AUTO_TEST_CASE(protocol_version_106__handshake__higher_peer_version__negotiated_lower)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::maximum_protocol));
    BOOST_REQUIRE_EQUAL(node_version->value, level::bip31);

    send(ping{ 42 }, level::bip31);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip31)->nonce, 42_u64);
}

BOOST_AUTO_TEST_CASE(protocol_version_106__handshake__self_configured__self_address_sender)
{
    static const network::config::address self{ "1.2.3.4:8333" };
    settings_.protocol_maximum = level::bip31;
    settings_.inbound.selfs.push_back(self);
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));
    BOOST_REQUIRE(node_version->address_sender.address == self.to_address_item(0, 0).address);
    BOOST_REQUIRE_EQUAL(node_version->address_sender.port, 8333_u16);
}

BOOST_AUTO_TEST_CASE(protocol_version_106__handshake__self_unconfigured__unspecified_address_sender)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));
    BOOST_REQUIRE(is_unspecified(node_version->address_sender.address));
    BOOST_REQUIRE_EQUAL(node_version->address_sender.port, unspecified_ip_port);
}

BOOST_AUTO_TEST_CASE(protocol_version_106__handshake__outbound__bip31_node_version)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(dial());
    BOOST_REQUIRE(handshake(level::bip31));
    BOOST_REQUIRE_EQUAL(node_version->value, level::bip31);
    BOOST_REQUIRE_EQUAL(node_version->address_receiver.port, 65031_u16);
}

BOOST_AUTO_TEST_CASE(protocol_version_106__shake__minimum_below_minimum_protocol__dropped)
{
    settings_.protocol_maximum = level::bip31;
    settings_.protocol_minimum = sub1(level::minimum_protocol);
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_106__shake__maximum_above_maximum_protocol__dropped)
{
    settings_.protocol_maximum = add1(level::maximum_protocol);
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_106__shake__minimum_above_maximum__dropped)
{
    settings_.protocol_maximum = level::bip31;
    settings_.protocol_minimum = level::bip37;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_106__shake__no_peer_version__timeout_dropped)
{
    settings_.protocol_maximum = level::bip31;
    settings_.handshake_timeout_seconds = 1;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_106__receive_version__invalid_services__dropped)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    send_version(level::bip31, settings_.invalid_services, network::unix_time());
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_106__receive_version__below_minimum__dropped)
{
    settings_.protocol_maximum = level::bip31;
    settings_.protocol_minimum = level::bip31;
    BOOST_REQUIRE(open());
    send_version(level::minimum_protocol, service::node_none, network::unix_time());
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_106__receive_version__timestamp_behind_skew__dropped)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    send_version(level::bip31, service::node_none, network::unix_time() - (settings_.maximum_skew_minutes + 10u) * 60u);
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_106__receive_version__timestamp_ahead_skew__dropped)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    send_version(level::bip31, service::node_none, network::unix_time() + (settings_.maximum_skew_minutes + 10u) * 60u);
    BOOST_REQUIRE(dropped());
}

BOOST_AUTO_TEST_CASE(protocol_version_106__receive_version__timestamp_within_skew__handshake)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    send_version(level::bip31, service::node_none, network::unix_time() - (settings_.maximum_skew_minutes - 10u) * 60u);
    BOOST_REQUIRE(receive_handshake(level::bip31));
    send(version_acknowledge{}, level::bip31);

    send(ping{ 42 }, level::bip31);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip31)->nonce, 42_u64);
}

BOOST_AUTO_TEST_CASE(protocol_version_106__receive_version__duplicate_after_handshake__ignored)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));
    send_version(level::bip31, service::node_none, network::unix_time());

    send(ping{ 42 }, level::bip31);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip31)->nonce, 42_u64);
}

BOOST_AUTO_TEST_CASE(protocol_version_106__receive_acknowledge__duplicate_after_handshake__ignored)
{
    settings_.protocol_maximum = level::bip31;
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));
    send(version_acknowledge{}, level::bip31);

    send(ping{ 42 }, level::bip31);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip31)->nonce, 42_u64);
}

BOOST_AUTO_TEST_SUITE_END()
