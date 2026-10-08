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

using namespace network::messages::peer;

// A ping protocol whose heartbeat timer is expired by the test.
class ping_106_probe
  : public protocol_ping_106
{
public:
    typedef std::shared_ptr<ping_106_probe> ptr;
    using protocol_ping_106::protocol_ping_106;

    void expire() NOEXCEPT
    {
        post<ping_106_probe>(&ping_106_probe::handle_timer, code{});
    }

    std::promise<bool> received{};

protected:
    // The subscription is also notified with an error code upon stop.
    bool handle_receive_ping(const code& ec,
        const ping::cptr& message) NOEXCEPT override
    {
        const auto result = protocol_ping_106::handle_receive_ping(ec, message);
        if (!ec && !received_)
        {
            received_ = true;
            received.set_value(true);
        }

        return result;
    }

private:
    bool received_{};
};

class ping_106_session
  : public session_inbound
{
public:
    ping_106_session(net& network, uint64_t identifier,
        std::promise<ping_106_probe::ptr>& probe) NOEXCEPT
      : session_inbound(network, identifier), probe_(probe)
    {
    }

protected:
    void attach_protocols(const channel::ptr& channel) NOEXCEPT override
    {
        const auto probe = channel->attach<ping_106_probe>(shared_from_this());
        probe_.set_value(probe);
        probe->start();
    }

private:
    std::promise<ping_106_probe::ptr>& probe_;
};

class ping_106_net
  : public net
{
public:
    using net::net;
    std::promise<ping_106_probe::ptr> probe{};

protected:
    session_inbound::ptr attach_inbound_session() NOEXCEPT override
    {
        return attach<ping_106_session>(*this, probe);
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_ping_106_tests, peer_net_setup_fixture<>)

BOOST_AUTO_TEST_CASE(protocol_ping_106__start__address_timestamp_peer__ping_without_nonce)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::address_timestamp));
    BOOST_REQUIRE(receive(ping::command).empty());
}

BOOST_FIXTURE_TEST_CASE(protocol_ping_106__handle_timer__expired__pings_repeated, peer_net_setup_fixture<ping_106_net>)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::address_timestamp));
    BOOST_REQUIRE(receive(ping::command).empty());

    const auto probe = net_->probe.get_future().get();
    probe->expire();
    BOOST_REQUIRE(receive(ping::command).empty());
    probe->expire();
    BOOST_REQUIRE(receive(ping::command).empty());
    probe->expire();
    BOOST_REQUIRE(receive(ping::command).empty());
}

BOOST_FIXTURE_TEST_CASE(protocol_ping_106__receive_ping__address_timestamp_peer__no_pong, peer_net_setup_fixture<ping_106_net>)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::address_timestamp));
    BOOST_REQUIRE(receive(ping::command).empty());

    // The expired ping follows the node's handling of the peer's ping.
    const auto probe = net_->probe.get_future().get();
    send(ping{}, level::address_timestamp);
    BOOST_REQUIRE(probe->received.get_future().get());
    probe->expire();
    BOOST_REQUIRE_EQUAL(receive().first, ping::command);
}

BOOST_AUTO_TEST_SUITE_END()
