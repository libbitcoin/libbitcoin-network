/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include "test.hpp"
#include <cstdio>
#include <future>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <bitcoin/network.hpp>

using namespace bc::network;
using namespace bc::system::chain;
using namespace bc::network::messages;

class mock_channel
  : public channel
{
public:
    using channel::channel;

    virtual void start() override
    {
        channel::start();
    }

    virtual void stop(const code& ec) override
    {
        channel::stop(ec);
    }

    // Override protected base capture sent payload.
    virtual void send(system::chunk_ptr payload,
        result_handler&& handler) override
    {
        payload_ = payload;
    }

    // Override protected base to notify subscribers.
    code notify(identifier id, uint32_t version, system::reader& source)
    {
        return notify(id, version, source);
    }

    // Get last captured payload.
    system::chunk_ptr sent() const
    {

        return payload_;
    }

private:
    system::chunk_ptr payload_;
};

// Use mock acceptor to inject mock channel.
class mock_acceptor
  : public acceptor
{
public:
    mock_acceptor(asio::strand& strand, asio::io_context& service,
        const settings& settings)
      : acceptor(strand, service, settings), stopped_(false), port_(0)
    {
    }

    // Get captured port.
    uint16_t port() const
    {
        return port_;
    }

    // Get captured stopped.
    bool stopped() const
    {
        return stopped_;
    }

    // Capture port.
    virtual code start(uint16_t port) override
    {
        port_ = port;
    }

    // Capture stopped.
    virtual void stop() override
    {
        stopped_ = true;
    }

    // Inject mock channel.
    virtual void accept(accept_handler&& handler) override
    {
        const auto socket = std::make_shared<network::socket>(service_);
        const auto created = std::make_shared<mock_channel>(socket, settings_);
        handler(error::success, created);
    }

private:
    bool stopped_;
    uint16_t port_;
};

// Use mock connector to inject mock channel.
class mock_connector
  : public connector
{
public:
    mock_connector(asio::strand& strand, asio::io_context& service,
        const settings& settings)
      : connector(strand, service, settings), stopped_(false)
    {
    }

    // Get captured stopped.
    bool stopped() const
    {
        return stopped_;
    }

    // Capture stopped.
    virtual void stop() override
    {
        stopped_ = true;
    }

    // Inject mock channel.
    virtual void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) override
    {
        const auto socket = std::make_shared<network::socket>(service_);
        const auto created = std::make_shared<mock_channel>(socket, settings_);
        handler(error::success, created);
    }

private:
    bool stopped_;
};

// Use mock p2p network to inject mock channels.
class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    ////virtual session_seed::ptr attach_seed_session() override
    ////{
    ////    return p2p::attach_seed_session();
    ////}
    ////
    ////virtual session_manual::ptr attach_manual_session() override
    ////{
    ////    return p2p::attach_manual_session();
    ////}
    ////
    ////virtual session_inbound::ptr attach_inbound_session() override
    ////{
    ////    return p2p::attach_inbound_session();
    ////}
    ////
    ////virtual session_outbound::ptr attach_outbound_session() override
    ////{
    ////    return p2p::attach_outbound_session();
    ////}

    // Create mock acceptor to inject mock channel.
    virtual acceptor::ptr create_acceptor() override
    {
        return std::make_shared<mock_acceptor>(strand(), service(),
            network_settings());
    }

    // Create mock connector to inject mock channel.
    virtual connector::ptr create_connector() override
    {
        return std::make_shared<mock_connector>(strand(), service(),
            network_settings());
    }
};

BOOST_AUTO_TEST_SUITE(p2p_tests)

BOOST_AUTO_TEST_CASE(p2p__network_settings__unstarted__expected)
{
    settings set(selection::mainnet);
    BOOST_REQUIRE_EQUAL(set.threads, 1u);

    p2p net(set);
    BOOST_REQUIRE_EQUAL(net.network_settings().threads, 1u);
}

BOOST_AUTO_TEST_CASE(p2p__address_count__unstarted__zero)
{
    const settings set(selection::mainnet);
    p2p net(set);
    BOOST_REQUIRE_EQUAL(net.address_count(), 0u);
}

BOOST_AUTO_TEST_CASE(p2p__channel_count__unstarted__zero)
{
    const settings set(selection::mainnet);
    p2p net(set);
    BOOST_REQUIRE_EQUAL(net.channel_count(), 0u);
}

BOOST_AUTO_TEST_CASE(p2p__connect__unstarted__service_stopped)
{
    const settings set(selection::mainnet);
    p2p net(set);

    std::promise<bool> promise;
    const auto handler = [&](const code& ec, channel::ptr channel)
    {
        BOOST_REQUIRE(!channel);
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise.set_value(true);
    };

    net.connect({ "truckers.ca" });
    net.connect("truckers.ca", 42);
    net.connect("truckers.ca", 42, handler);
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__subscribe_connect__unstarted__success)
{
    const settings set(selection::mainnet);
    p2p net(set);

    std::promise<bool> promise_handler;
    const auto handler = [&](const code& ec, channel::ptr channel)
    {
        BOOST_REQUIRE(!channel);
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise_handler.set_value(true);
    };

    std::promise<bool> promise_complete;
    const auto complete = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_complete.set_value(true);
    };

    net.subscribe_connect(handler, complete);
    BOOST_REQUIRE(promise_complete.get_future().get());

    // Close (or ~p2p) required to clear subscription.
    net.close();
    BOOST_REQUIRE(promise_handler.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__subscribe_close__unstarted__service_stopped)
{
    const settings set(selection::mainnet);
    p2p net(set);

    std::promise<bool> promise_handler;
    const auto handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise_handler.set_value(true);
    };

    std::promise<bool> promise_complete;
    const auto complete = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_complete.set_value(true);
    };

    net.subscribe_close(handler, complete);
    BOOST_REQUIRE(promise_complete.get_future().get());

    // Close (or ~p2p) required to clear subscription.
    net.close();
    BOOST_REQUIRE(promise_handler.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__start__no__peers_no_seeds__success)
{
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());
    set.seeds.clear();

    p2p net(set);
    BOOST_REQUIRE(net.network_settings().peers.empty());
    BOOST_REQUIRE(net.network_settings().seeds.empty());

    std::promise<bool> promise;
    const auto handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise.set_value(true);
    };

    net.start(handler);
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__run__not_started__service_stopped)
{
    settings set(selection::mainnet);
    p2p net(set);

    std::promise<bool> promise;
    const auto handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::service_stopped);
        promise.set_value(true);
    };

    net.run(handler);
    BOOST_REQUIRE(promise.get_future().get());
}

BOOST_AUTO_TEST_CASE(p2p__run__started_no_peers_no_seeds__success)
{
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.peers.empty());
    set.seeds.clear();

    p2p net(set);
    BOOST_REQUIRE(net.network_settings().peers.empty());
    BOOST_REQUIRE(net.network_settings().seeds.empty());

    std::promise<bool> promise_run;
    const auto run_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        promise_run.set_value(true);
    };

    std::promise<bool> promise_start;
    const auto start_handler = [&](const code& ec)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        net.run(run_handler);
    };

    net.start(start_handler);
    BOOST_REQUIRE(promise_run.get_future().get());
}

template<class Message>
static int send_result(const Message& message, p2p& network, int channels)
{
    const auto channel_counter = [&channels](code ec, channel::ptr)
    {
        BOOST_REQUIRE_EQUAL(ec, error::success);
        --channels;
    };

    std::promise<code> promise;
    const auto completion_handler = [&promise](code ec)
    {
        promise.set_value(ec);
    };

    network.broadcast(message, channel_counter, completion_handler);
    const auto result = promise.get_future().get().value();

    BOOST_REQUIRE_EQUAL(channels, 0);
    return result;
}

BOOST_AUTO_TEST_SUITE_END()
