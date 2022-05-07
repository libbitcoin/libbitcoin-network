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
using namespace bc::network::config;
using namespace bc::system::chain;

class mock_connector_connect_success
  : public connector
{
public:
    typedef std::shared_ptr<mock_connector_connect_success> ptr;

    using connector::connector;

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
    void stop() noexcept override
    {
        stopped_ = true;
        connector::stop();
    }

    // Handle connect, capture first connected hostname and port.
    void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) noexcept override
    {
        if (is_zero(connects_++))
        {
            hostname_ = hostname;
            port_ = port;
        }

        const auto socket = std::make_shared<network::socket>(service_);
        const auto channel = std::make_shared<network::channel>(socket, settings_);

        // Must be asynchronous or is an infinite recursion.
        boost::asio::post(strand_, [=]()
        {
            // Connect result code is independent of the channel stop code.
            // As error code woulod set the re-listener timer, channel pointer is ignored.
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

    using mock_connector_connect_success::mock_connector_connect_success;

    // Handle connect with service_stopped error.
    void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) noexcept override
    {
        if (is_zero(connects_++))
        {
            hostname_ = hostname;
            port_ = port;
        }

        boost::asio::post(strand_, [=]()
        {
            // This error is eaten by handle_connect, due to retry logic.
            // invalid_magic is a non-terminal code (timer retry).
            // Connect result code is independent of the channel stop code.
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
    typename Connector::ptr get_connector() const
    {
        return connector_;
    }

    // Create mock connector to inject mock channel.
    connector::ptr create_connector() noexcept override
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
    using session_manual::session_manual;

    bool inbound() const noexcept override
    {
        return session_manual::inbound();
    }

    bool notify() const noexcept override
    {
        return session_manual::notify();
    }

    bool stopped() const
    {
        return session_manual::stopped();
    }

    authority start_connect_authority() const
    {
        return start_connect_authority_;
    }

    // Capture first start_connect call.
    void start_connect(const authority& host, connector::ptr connector,
        channel_handler handler) noexcept override
    {
        // Must be first to ensure connector::connect() preceeds promise release.
        session_manual::start_connect(host, connector, handler);

        if (is_one(connects_))
            reconnect_.set_value(true);

        if (is_zero(connects_++))
        {
            start_connect_authority_ = host;
            connect_.set_value(true);
        }
    }

    bool connected() const
    {
        return !is_zero(connects_);
    }

    bool require_connected() const
    {
        return connect_.get_future().get();
    }

    bool require_reconnect() const
    {
        return reconnect_.get_future().get();
    }

    void attach_handshake(const channel::ptr&,
        result_handler handshake) const noexcept override
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

protected:
    mutable bool handshaked_{ false };
    mutable std::promise<bool> handshake_;

private:
    code connect_code_{ error::success };
    authority start_connect_authority_;
    size_t connects_{ zero };
    mutable std::promise<bool> connect_;
    mutable std::promise<bool> reconnect_;
};

class mock_session_manual_handshake_failure
  : public mock_session_manual
{
public:
    using mock_session_manual::mock_session_manual;

    void attach_handshake(const channel::ptr&,
        result_handler handshake) const noexcept override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
            handshake_.set_value(true);
        }

        // Simulate handshake failure.
        handshake(error::invalid_checksum);
    }
};

// properties

BOOST_AUTO_TEST_CASE(session_manual__inbound__always__false)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session_manual session(net);
    BOOST_REQUIRE(!session.inbound());
}

BOOST_AUTO_TEST_CASE(session_manual__notify__always__true)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session_manual session(net);
    BOOST_REQUIRE(session.notify());
}

// stop

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

// connect

BOOST_AUTO_TEST_CASE(session_manual__connect1__stopped__service_stopped)
{
    settings set(selection::mainnet);
    mock_p2p<network::connector> net(set);
    auto session = std::make_shared<mock_session_manual>(net);
    BOOST_REQUIRE(session->stopped());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

    boost::asio::post(net.strand(), [=]()
    {
        // This synchronous overload has no handler, so cannot capture values.
        session->connect(hostname, port);
    });

    // No handler so rely on connect.
    BOOST_REQUIRE(session->require_connected());

    // A connector was created/subscribed, which requires unstarted service stop.
    BOOST_REQUIRE(net.get_connector());

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

BOOST_AUTO_TEST_CASE(session_manual__connect2__stopped__service_stopped)
{
    settings set(selection::mainnet);
    mock_p2p<network::connector> net(set);
    auto session = std::make_shared<mock_session_manual>(net);
    BOOST_REQUIRE(session->stopped());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

    std::promise<code> connected;
    boost::asio::post(net.strand(), [=, &connected]()
    {
        session->connect(hostname, port, [&](const code& ec, channel::ptr channel)
        {
            BOOST_REQUIRE(!channel);
            connected.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(connected.get_future().get(), error::service_stopped);

    // A connector was created/subscribed, which requires unstarted service stop.
    BOOST_REQUIRE(net.get_connector());

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

BOOST_AUTO_TEST_CASE(session_manual__connect3__stopped__service_stopped)
{
    settings set(selection::mainnet);
    mock_p2p<network::connector> net(set);
    auto session = std::make_shared<mock_session_manual>(net);
    BOOST_REQUIRE(session->stopped());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

    std::promise<code> connected;
    boost::asio::post(net.strand(), [=, &connected]()
    {
        session->connect({ hostname, port }, [&](const code& ec, channel::ptr channel)
        {
            BOOST_REQUIRE(!channel);
            connected.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(connected.get_future().get(), error::service_stopped);

    // A connector was created/subscribed, which requires unstarted service stop.
    BOOST_REQUIRE(net.get_connector());

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

BOOST_AUTO_TEST_CASE(session_manual__handle_connect__connect_fail__service_stopped)
{
    settings set(selection::mainnet);
    mock_p2p<mock_connector_connect_fail> net(set);
    auto session = std::make_shared<mock_session_manual>(net);
    BOOST_REQUIRE(session->stopped());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

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

    std::promise<bool> started_connect;
    std::promise<code> connected;
    boost::asio::post(net.strand(), [=, &connected, &started_connect]()
    {
        session->connect({ hostname, port }, [&](const code& ec, channel::ptr channel)
        {
            BOOST_REQUIRE(!channel);
            connected.set_value(ec);
        });

        // connector.connect has been invoked, though its handler is pending.
        started_connect.set_value(true);
    });

    BOOST_REQUIRE(started_connect.get_future().get());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    // connector.connect sets invalid_magic, causing a timer reconnect.
    // session_manual always sets service_stopped, with all other codes eaten.
    BOOST_REQUIRE_EQUAL(connected.get_future().get(), error::service_stopped);

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_manual__handle_connect__connect_success_stopped__service_stopped)
{
    settings set(selection::mainnet);
    mock_p2p<mock_connector_connect_success> net(set);
    auto session = std::make_shared<mock_session_manual>(net);
    BOOST_REQUIRE(session->stopped());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";
    const authority expected{ hostname, port };

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

    std::promise<bool> stopped;
    std::promise<code> connected;
    boost::asio::post(net.strand(), [=, &stopped, &connected]()
    {
        session->connect({ hostname, port }, [&](const code& ec, channel::ptr channel)
        {
            BOOST_REQUIRE(!channel);
            connected.set_value(ec);
        });

        // connector.connect has been invoked, though its handler is pending.
        // Stop the session after connect but before handle_connect is invoked.
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE_EQUAL(connected.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(session->require_connected());
    BOOST_REQUIRE_EQUAL(session->start_connect_authority(), expected);
    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_manual__handle_channel_start__handshake_error__invalid_checksum)
{
    settings set(selection::mainnet);
    mock_p2p<mock_connector_connect_success> net(set);
    auto session = std::make_shared<mock_session_manual_handshake_failure>(net);
    BOOST_REQUIRE(session->stopped());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";
    const authority expected{ hostname, port };

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

    auto first = true;
    std::promise<code> connected;
    boost::asio::post(net.strand(), [=, &first, &connected]()
    {
        session->connect({ hostname, port }, [&](const code& ec, channel::ptr channel)
        {
            if (first)
            {
                first = false;
                BOOST_REQUIRE(channel);
                connected.set_value(ec);
            }
        });
    });

    // mock_session_manual_handshake_failure sets channel.stop(invalid_checksum).
    BOOST_REQUIRE_EQUAL(connected.get_future().get(), error::invalid_checksum);
    BOOST_REQUIRE(session->require_connected());
    BOOST_REQUIRE_EQUAL(session->start_connect_authority(), expected);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(session->attached_handshake());
    session.reset();
}

// start via network (not required for coverage)

BOOST_AUTO_TEST_CASE(session_manual__start__network_start_no_seeds__success)
{
    settings set(selection::mainnet);

    // Preclude seeding.
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

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_no_connections__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    // Connector is not invoked.
    mock_p2p<connector> net(set);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            run.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_configured_connection__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";
    set.peers.push_back({ hostname, port });

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);

    // Connector is established and connect is called for all configured
    // connections prior to completion of network run call.
    BOOST_REQUIRE(net.get_connector());
    BOOST_REQUIRE_EQUAL(net.get_connector()->port(), port);
    BOOST_REQUIRE_EQUAL(net.get_connector()->hostname(), hostname);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_configured_connections__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
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

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);

    // Connector is established and connect is called for all configured
    // connections prior to completion of network run call. The last connection
    // is reflected by the mock connector as connections invoked in order.
    BOOST_REQUIRE(net.get_connector());
    BOOST_REQUIRE_EQUAL(net.get_connector()->port(), port);
    BOOST_REQUIRE_EQUAL(net.get_connector()->hostname(), hostname);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect1__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            net.connect(endpoint{ hostname, port });
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_connector()->hostname(), hostname);
    BOOST_REQUIRE_EQUAL(net.get_connector()->port(), port);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect2__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            net.connect(hostname, port);
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_connector()->hostname(), hostname);
    BOOST_REQUIRE_EQUAL(net.get_connector()->port(), port);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect3__success)
{
    settings set(selection::mainnet);

    // Preclude seeding, inbound, outbound and no manual connections.
    set.inbound_port = 0;
    set.seeds.clear();
    BOOST_REQUIRE(set.peers.empty());

    const uint16_t port = 42;
    const auto hostname = "42.42.42.42";

    // Connect will return invalid_magic when executed.
    mock_p2p<mock_connector_connect_fail> net(set);

    std::promise<code> start;
    std::promise<code> run;
    std::promise<code> connect;
    channel::ptr connected_channel;
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec)
        {
            net.connect(hostname, port, [&](const code& ec, channel::ptr channel)
            {
                connected_channel = channel;
                connect.set_value(ec);
            });

            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);

    // The connection loops on connect failure until service stop.
    net.close();
    BOOST_REQUIRE(!connected_channel);
    BOOST_REQUIRE_EQUAL(connect.get_future().get(), error::service_stopped);
    BOOST_REQUIRE_EQUAL(net.get_connector()->hostname(), hostname);
    BOOST_REQUIRE_EQUAL(net.get_connector()->port(), port);
}

BOOST_AUTO_TEST_SUITE_END()
