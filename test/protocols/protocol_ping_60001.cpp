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
class ping_60001_probe
  : public protocol_ping_60001
{
public:
    typedef std::shared_ptr<ping_60001_probe> ptr;
    using protocol_ping_60001::protocol_ping_60001;

    void expire() NOEXCEPT
    {
        post<ping_60001_probe>(&ping_60001_probe::handle_timer, code{});
    }
};

class ping_60001_session
  : public session_inbound
{
public:
    ping_60001_session(net& network, uint64_t identifier,
        std::promise<ping_60001_probe::ptr>& probe) NOEXCEPT
      : session_inbound(network, identifier), probe_(probe)
    {
    }

protected:
    void attach_protocols(const channel::ptr& channel) NOEXCEPT override
    {
        const auto probe = channel->attach<ping_60001_probe>(shared_from_this());
        probe_.set_value(probe);
        probe->start();
    }

private:
    std::promise<ping_60001_probe::ptr>& probe_;
};

class ping_60001_net
  : public net
{
public:
    using net::net;
    std::promise<ping_60001_probe::ptr> probe{};

protected:
    session_inbound::ptr attach_inbound_session() NOEXCEPT override
    {
        return attach<ping_60001_session>(*this, probe);
    }
};

BOOST_FIXTURE_TEST_SUITE(protocol_ping_60001_tests, peer_net_setup_fixture<>)

BOOST_AUTO_TEST_CASE(protocol_ping_60001__start__bip31_peer__ping_with_nonzero_nonce)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));

    const auto message = receive<ping>(level::bip31);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_NE(message->nonce, 0_u64);
}

BOOST_AUTO_TEST_CASE(protocol_ping_60001__receive_pong__matching_nonce__connected)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));

    const auto message = receive<ping>(level::bip31);
    BOOST_REQUIRE(message);
    send(pong{ message->nonce }, level::bip31);

    send(ping{ 42 }, level::bip31);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip31)->nonce, 42_u64);
}

BOOST_FIXTURE_TEST_CASE(protocol_ping_60001__handle_timer__expired_no_pong__dropped, peer_net_setup_fixture<ping_60001_net>)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));
    BOOST_REQUIRE(receive<ping>(level::bip31));

    net_->probe.get_future().get()->expire();
    BOOST_REQUIRE(dropped());
}

BOOST_FIXTURE_TEST_CASE(protocol_ping_60001__handle_timer__expired_after_pong__ping_again, peer_net_setup_fixture<ping_60001_net>)
{
    BOOST_REQUIRE(open());
    BOOST_REQUIRE(handshake(level::bip31));

    const auto first = receive<ping>(level::bip31);
    BOOST_REQUIRE(first);
    send(pong{ first->nonce }, level::bip31);

    // The pong echo orders the node's handling of the peer's pong.
    send(ping{ 42 }, level::bip31);
    BOOST_REQUIRE_EQUAL(receive<pong>(level::bip31)->nonce, 42_u64);

    net_->probe.get_future().get()->expire();
    const auto next = receive<ping>(level::bip31);
    BOOST_REQUIRE(next);
    BOOST_REQUIRE_NE(next->nonce, 0_u64);
}

BOOST_AUTO_TEST_SUITE_END()
