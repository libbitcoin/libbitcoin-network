/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(session_tests)

using namespace bc::network;
using namespace bc::system::chain;
using namespace bc::network::messages;

class mock_session
  : public session
{
public:
    mock_session(p2p& network)
      : session(network)
    {
    }

    bool stopped() const noexcept
    {
        return session::stopped();
    }

    bool inbound() const noexcept
    {
        return session::inbound();
    }

    void start_channel(channel::ptr channel, result_handler started,
        result_handler stopped)
    {
        session::start_channel(channel, started, stopped);
    }

    // Handshake

    bool attached_handshake() const
    {
        return handshaked_;
    }

    bool require_attached_handshake() const
    {
        return handshake_.get_future().get();
    }

    void attach_handshake(const channel::ptr& channel,
        result_handler handshake) const override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
            handshake_.set_value(true);
        }

        // Simulate handshake successful completion.
        handshake(error::success);
    }

    // Protocols

    bool attached_protocol() const
    {
        return protocoled_;
    }

    bool require_attached_protocol() const
    {
        return protocols_.get_future().get();
    }

    void attach_protocols(const channel::ptr&) const override
    {
        if (!protocoled_)
        {
            protocoled_ = true;
            protocols_.set_value(true);
        }
    }

private:
    mutable bool handshaked_{ false };
    mutable bool protocoled_{ false };
    mutable std::promise<bool> handshake_;
    mutable std::promise<bool> protocols_;
};

// construct/settings

BOOST_AUTO_TEST_CASE(session__construct__always__expected_settings)
{
    constexpr auto expected = 42u;
    settings set(selection::mainnet);
    set.threads = expected;
    p2p net(set);
    mock_session session(net);
    BOOST_REQUIRE_EQUAL(session.settings().threads, expected);
}

// start/stop

BOOST_AUTO_TEST_CASE(session__stop__stopped__true)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session session(net);
    std::promise<code> started;
    session.start([&](const code& ec)
    {
        started.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(session__stop__started__stopped)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session session(net);

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        session.start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

// start_channel

BOOST_AUTO_TEST_CASE(session__start_channel__started_outbound_channel_started_handshake_success_network_stopped__handlers_success_channel_stopped)
{
    settings set(selection::mainnet);
    p2p net(set);
    auto session = std::make_shared<mock_session>(net);

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.service());
    const auto channel = std::make_shared<network::channel>(socket,
        session->settings());

    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    BOOST_REQUIRE(session->require_attached_handshake());
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);

    // Channel stopped by network.store failure (network stopped).
    BOOST_REQUIRE(channel->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

// attach_handshake
// attach_protocols
// create_acceptor
// create_connector
// create_connectors

// stopped
// blacklisted
// stranded
// address_count
// channel_count
// inbound_channel_count
// inbound

BOOST_AUTO_TEST_CASE(session__inbound__always__false)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session session(net);
    BOOST_REQUIRE(!session.inbound());
}

// notify

// fetch
// fetches
// save
// saves



BOOST_AUTO_TEST_SUITE_END()
