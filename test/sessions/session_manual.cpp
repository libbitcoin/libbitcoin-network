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

BOOST_AUTO_TEST_SUITE(session_manual_tests)

using namespace bc::network;
using namespace bc::system::chain;
using namespace bc::network::config;
using namespace bc::network::messages;


class mock_connector_connect_success
  : public connector
{
public:
    typedef std::shared_ptr<mock_connector_connect_success> ptr;

    mock_connector_connect_success(asio::strand& strand,
        asio::io_context& service, const settings& settings)
      : connector(strand, service, settings)
    {
    }

    // Get captured connected.
    virtual bool connected() const
    {
        return !is_zero(connects_);
    }

    // Get captured hostname.
    virtual std::string hostname() const
    {
        return hostname_;
    }

    // Get captured port.
    virtual uint16_t port() const
    {
        return port_;
    }

    // Get captured stopped.
    virtual bool stopped() const
    {
        return stopped_;
    }

    // Capture stopped and free channel.
    void stop() override
    {
        stopped_ = true;
        connector::stop();
    }

    // Handle connect (other connect overloads pass to this).
    void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) override
    {
        if (is_zero(connects_++))
        {
            hostname_ = hostname;
            port_ = port;
        }

        const auto socket = std::make_shared<network::socket>(service_);
        const auto channel = std::make_shared<network::channel>(socket, settings_);

        // Must be asynchronous or is an infinite recursion.
        // This error code will set the re-listener timer and channel pointer is ignored.
        boost::asio::post(strand_, [=]()
        {
            handler(error::success, channel);
        });
    }

protected:
    bool stopped_{ false };
    size_t connects_{ zero };
    std::string hostname_;
    uint16_t port_;
};

class mock_connector_connect_fail
  : public mock_connector_connect_success
{
public:
    typedef std::shared_ptr<mock_connector_connect_fail> ptr;

    mock_connector_connect_fail(asio::strand& strand,
        asio::io_context& service, const settings& settings)
      : mock_connector_connect_success(strand, service, settings)
    {
    }

    // Handle connect with invalid_magic error.
    void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) override
    {
        if (is_zero(connects_++))
        {
            hostname_ = hostname;
            port_ = port;
        }

        boost::asio::post(strand_, [=]()
        {
            // This error is eaten by handle_connect, due to retry logic.
            handler(error::invalid_magic, nullptr);
        });
    }
};

template <class Connector>
class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    // Get last created connector.
    typename Connector::ptr connector() const
    {
        return connector_;
    }

    // Create mock connector to inject mock channel.
    connector::ptr create_connector() override
    {
        return ((connector_ = std::make_shared<Connector>(strand(), service(),
            network_settings())));
    }

private:
    typename Connector::ptr connector_;
};

class mock_session_manual
  : public session_manual
{
public:
    mock_session_manual(p2p& network)
      : session_manual(network)
    {
    }

    bool inbound() const noexcept override
    {
        return session_manual::inbound();
    }

    bool stopped() const
    {
        return session_manual::stopped();
    }

    void start_connect(const authority& host, connector::ptr connector,
        channel_handler handler) override
    {
        // Must be first to ensure connector::connect() preceeds promise release.
        session_manual::start_connect(host, connector, handler);

        if (!connected_)
        {
            connected_ = true;
            connect_.set_value(true);
        }
    }

    bool connected() const
    {
        return connected_;
    }

    bool require_connected() const
    {
        return connect_.get_future().get();
    }

    void attach_handshake(const channel::ptr&,
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

    bool attached_handshake() const
    {
        return handshaked_;
    }

    bool require_attached_handshake() const
    {
        return handshake_.get_future().get();
    }

private:
    mutable bool connected_{ false };
    mutable bool handshaked_{ false };
    mutable std::promise<bool> connect_;
    mutable std::promise<bool> handshake_;
};

// inbound

BOOST_AUTO_TEST_CASE(session_manual__inbound__always__false)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session_manual session(net);
    BOOST_REQUIRE(!session.inbound());
}

// start/stop

BOOST_AUTO_TEST_CASE(session_manual__stop__started__stopped)
{
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    p2p net(set);
    auto session = std::make_shared<mock_session_manual>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        // Will cause started to be set (only).
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_manual__stop__stopped__stopped)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session_manual session(net);

    std::promise<bool> promise;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        promise.set_value(true);
    });

    BOOST_REQUIRE(promise.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

// start

BOOST_AUTO_TEST_CASE(session_manual__start__started__operation_failed)
{
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    p2p net(set);
    auto session = std::make_shared<mock_session_manual>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<code> restart;
    boost::asio::post(net.strand(), [=, &restart]()
    {
        session->start([&](const code& ec)
        {
            restart.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(restart.get_future().get(), error::operation_failed);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

// start via network

BOOST_AUTO_TEST_CASE(session_manual__start__network_start_no_seeds__success)
{
    settings set(selection::mainnet);

    // Preclude seeding.
    set.host_pool_capacity = 0;
    set.seeds.clear();

    // Connector is not invoked.
    mock_p2p<connector> net(set);

    std::promise<code> started;
    net.start([&](const code& ec)
    {
        started.set_value(ec);
    });
    
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_no_conections__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.inbound_connections = 0;
    set.host_pool_capacity = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    // Connector is not invoked.
    mock_p2p<connector> net(set);

    std::promise<code> promise_start;
    std::promise<code> promise_run;
    net.start([&](const code& ec)
    {
        promise_start.set_value(ec);
        net.run([&](const code& ec)
        {
            promise_run.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(promise_start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(promise_run.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_configured_connection__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.inbound_connections = 0;
    set.host_pool_capacity = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";
    set.peers.push_back({ hostname, port });

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set);

    std::promise<code> promise_start;
    std::promise<code> promise_run;
    net.start([&](const code& ec)
    {
        promise_start.set_value(ec);
        net.run([&](const code& ec)
        {
            promise_run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(promise_start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(promise_run.get_future().get(), error::success);

    // Connector is established and connect is called for all configured
    // connections prior to completion of network run call.
    BOOST_REQUIRE(net.connector());
    BOOST_REQUIRE_EQUAL(net.connector()->port(), port);
    BOOST_REQUIRE_EQUAL(net.connector()->hostname(), hostname);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_configured_connections__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.inbound_connections = 0;
    set.host_pool_capacity = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.4";
    set.peers.push_back({ "42.42.42.1", 42 });
    set.peers.push_back({ "42.42.42.2", 42 });
    set.peers.push_back({ "42.42.42.3", 42 });
    set.peers.push_back({ hostname, port });

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set);

    std::promise<code> promise_start;
    std::promise<code> promise_run;
    net.start([&](const code& ec)
    {
        promise_start.set_value(ec);
        net.run([&](const code& ec)
        {
            promise_run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(promise_start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(promise_run.get_future().get(), error::success);

    // Connector is established and connect is called for all configured
    // connections prior to completion of network run call. The last connection
    // is reflected by the mock connector as connections invoked in order.
    BOOST_REQUIRE(net.connector());
    BOOST_REQUIRE_EQUAL(net.connector()->port(), port);
    BOOST_REQUIRE_EQUAL(net.connector()->hostname(), hostname);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect1__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.inbound_connections = 0;
    set.host_pool_capacity = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set);

    std::promise<code> promise_start;
    std::promise<code> promise_run;
    net.start([&](const code& ec)
    {
        promise_start.set_value(ec);
        net.run([&](const code& ec)
        {
            net.connect(endpoint{ hostname, port });
            promise_run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(promise_start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(promise_run.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.connector()->hostname(), hostname);
    BOOST_REQUIRE_EQUAL(net.connector()->port(), port);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect2__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.inbound_connections = 0;
    set.host_pool_capacity = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set);

    std::promise<code> promise_start;
    std::promise<code> promise_run;
    net.start([&](const code& ec)
    {
        promise_start.set_value(ec);
        net.run([&](const code& ec)
        {
            net.connect(hostname, port);
            promise_run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(promise_start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(promise_run.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.connector()->hostname(), hostname);
    BOOST_REQUIRE_EQUAL(net.connector()->port(), port);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect3__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.inbound_connections = 0;
    set.host_pool_capacity = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set);

    std::promise<code> promise_start;
    std::promise<code> promise_run;
    std::promise<code> promise_connect;
    channel::ptr connected_channel;
    net.start([&](const code& ec)
    {
        promise_start.set_value(ec);
        net.run([&](const code& ec)
        {
            net.connect(hostname, port, [&](const code& ec, channel::ptr channel)
            {
                connected_channel = channel;
                promise_connect.set_value(ec);
            });

            promise_run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(promise_start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(promise_run.get_future().get(), error::success);

    // The connection loops on connect failure until service stop.
    net.close();
    BOOST_REQUIRE(!connected_channel);
    BOOST_REQUIRE_EQUAL(promise_connect.get_future().get(), error::service_stopped);
    BOOST_REQUIRE_EQUAL(net.connector()->hostname(), hostname);
    BOOST_REQUIRE_EQUAL(net.connector()->port(), port);
}

// TODO: test connect with service stopped.
// session_manual::handle_connect (133)
// session_manual::handle_connect (145)

// TODO: test connect success scenarios.
// session_manual::handle_connect (151)

BOOST_AUTO_TEST_SUITE_END()
