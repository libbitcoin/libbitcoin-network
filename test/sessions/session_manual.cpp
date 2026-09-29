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

BOOST_AUTO_TEST_SUITE(session_manual_tests)

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace bc::network::config;
using namespace bc::system::chain;

class mock_connector_connect_success
  : public connector
{
public:
    typedef std::shared_ptr<mock_connector_connect_success> ptr;

    using connector::connector;

    // Get captured connected.
    bool connected_() const NOEXCEPT
    {
        return !is_zero(connects_);
    }

    // Get captured endpoint.
    const endpoint& peer() const NOEXCEPT
    {
        return peer_;
    }

    // Get captured stopped.
    bool stopped() const NOEXCEPT
    {
        return stopped_;
    }

    // Capture stopped and free channel.
    void stop() NOEXCEPT override
    {
        stopped_ = true;
        connector::stop();
    }

    // Handle connect, capture first connected hostname and port.
    void connect(const endpoint& peer,
        socket_handler&& handler) NOEXCEPT override
    {
        if (is_zero(connects_++))
            peer_ = peer;

        socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
        const auto socket = std::make_shared<network::socket>(log, service_,
            std::move(params));

        // Must be asynchronous or is an infinite recursion.
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            // Connect result code is independent of channel stop code.
            // Error code would set re-listener timer, channel pointer ignored.
            handler(error::success, socket);
        });
    }

protected:
    bool stopped_{ false };
    size_t connects_{ zero };
    endpoint peer_;
};

class mock_connector_connect_fail
  : public mock_connector_connect_success
{
public:
    typedef std::shared_ptr<mock_connector_connect_fail> ptr;

    using mock_connector_connect_success::mock_connector_connect_success;

    // Handle connect with service_stopped error.
    void connect(const endpoint& peer,
        socket_handler&& handler) NOEXCEPT override
    {
        if (is_zero(connects_++))
            peer_ = peer;

        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            // This error is eaten by handle_connect, due to retry logic.
            // invalid_magic is a non-terminal code (timer retry).
            // Connect result code is independent of the channel stop code.
            handler(error::invalid_magic, nullptr);
        });
    }
};

class mock_session_manual
  : public session_manual
{
public:
    using session_manual::session_manual;

    bool stopped() const NOEXCEPT override
    {
        return session_manual::stopped();
    }

    void defer(result_handler&& handler) NOEXCEPT override
    {
        session_manual::defer(std::move(handler));
    }

    const endpoint& start_connect_endpoint() const NOEXCEPT
    {
        return start_connect_endpoint_;
    }

    // Capture first start_connect call.
    void start_connect(const code&, const endpoint& peer,
        const connector::ptr& connector,
        const net::channel_notifier& handler, object_key key) NOEXCEPT override
    {
        // Must be first to ensure connector::start_connect() preceeds promise release.
        session_manual::start_connect({}, peer, connector, handler, key);

        if (is_one(connects_))
            reconnect_.set_value(true);

        if (is_zero(connects_++))
        {
            start_connect_endpoint_ = peer;
            connect_.set_value(true);
        }
    }

    bool connected_() const NOEXCEPT
    {
        return !is_zero(connects_);
    }

    bool require_connected() const NOEXCEPT
    {
        return connect_.get_future().get();
    }

    bool require_reconnect() const NOEXCEPT
    {
        return reconnect_.get_future().get();
    }

    void attach_handshake(const channel::ptr& channel,
        result_handler&& handshake) NOEXCEPT override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
            handshake_.set_value(true);
        }

        // The handshake protocol pauses the channel upon completion, which is
        // after the session resumes it to start the read loop, so this posts.
        boost::asio::post(channel->strand(),
            [channel, complete = std::move(handshake)]() NOEXCEPT
            {
                channel->pause();
                complete(error::success);
            });
    }

    bool attached_handshake() const NOEXCEPT
    {
        return handshaked_;
    }

    bool require_attached_handshake() const NOEXCEPT
    {
        return handshake_.get_future().get();
    }

protected:
    mutable bool handshaked_{ false };
    mutable std::promise<bool> handshake_;

private:
    code connect_code_{ error::success };
    endpoint start_connect_endpoint_;
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
        result_handler&& handshake) NOEXCEPT override
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

template <class Connector = connector>
class mock_net
  : public net
{
public:
    using net::net;

    // Get last created connector.
    typename Connector::ptr get_connector() const NOEXCEPT
    {
        return connector_;
    }

    // Create mock connector to inject mock channel.
    connector::ptr to_connector(const settings::socks5& ,
        const settings::tcp_server& options,
        const steady_clock::duration& timeout, size_t) NOEXCEPT override
    {
        connector::parameters params
        {
            .connect_timeout = timeout,
            .maximum_request = options.maximum_request
        };

        return ((connector_ = std::make_shared<Connector>(log, strand(),
            service(), suspended_, std::move(params))));
    }

    session_inbound::ptr attach_inbound_session() NOEXCEPT override
    {
        return attach<mock_inbound_session>(*this);
    }

    session_outbound::ptr attach_outbound_session() NOEXCEPT override
    {
        return attach<mock_outbound_session>(*this);
    }

    session_seed::ptr attach_seed_session() NOEXCEPT override
    {
        return attach<mock_seed_session>(*this);
    }

private:
    typename Connector::ptr connector_;
    std::atomic_bool suspended_{ false };

    class mock_inbound_session
      : public session_inbound
    {
    public:
        using session_inbound::session_inbound;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };

    class mock_outbound_session
      : public session_outbound
    {
    public:
        using session_outbound::session_outbound;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };

    class mock_seed_session
      : public session_seed
    {
    public:
        using session_seed::session_seed;

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };
};

// stop

BOOST_AUTO_TEST_CASE(session_manual__stop__started__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        // Will cause started to be set (only).
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__stop__stopped__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);
    mock_session_manual session(net, 1);

    std::promise<bool> promise;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
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
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<code> restart;
    boost::asio::post(net.strand(), [=, &restart]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            restart.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(restart.get_future().get(), error::operation_failed);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

// connect

BOOST_AUTO_TEST_CASE(session_manual__connect_unhandled__stopped__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint peer{ "42.42.42.42", 42 };

    boost::asio::post(net.strand(), [=]() NOEXCEPT
    {
        // This synchronous overload has no handler, so cannot capture values.
        session->connect(peer);
    });

    // No handler so rely on connect.
    BOOST_REQUIRE(session->require_connected());

    // A connector was created/subscribed, which requires unstarted service stop.
    BOOST_REQUIRE(net.get_connector());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__connect_handled__stopped__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint peer{ "42.42.42.42", 42 };

    std::promise<std::pair<code, channel::ptr>> connected;
    boost::asio::post(net.strand(), [=, &connected]() NOEXCEPT
    {
        session->connect(peer, [&](const code& ec, const channel::ptr& channel)
        {
            connected.set_value({ ec, channel });
            return true;
        });
    });

    const auto result = connected.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::service_stopped);
    BOOST_REQUIRE(!result.second);

    // A connector was created/subscribed, which requires unstarted service stop.
    BOOST_REQUIRE(net.get_connector());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__handle_connect__connect_fail__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);

    // Connect will return invalid_magic when executed.
    mock_net<mock_connector_connect_fail> net(set, log);

    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint peer{ "42.42.42.42", 42 };

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    auto first = true;
    std::promise<bool> started_connect;
    std::promise<std::pair<code, channel::ptr>> connected;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session->connect(peer, [&](const code& ec, const channel::ptr& channel) NOEXCEPT
        {
            if (first)
            {
                connected.set_value({ ec, channel });
                first = false;
            }

            // Continue after connect fail, reenters here.
            return true;
        });

        // connector.connect has been invoked, though its handler is pending.
        started_connect.set_value(true);
    });

    BOOST_REQUIRE(started_connect.get_future().get());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    // connector.connect sets invalid_magic, causing a timer reconnect.
    const auto result = connected.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::invalid_magic);
    BOOST_REQUIRE(!result.second);

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__handle_connect__connect_success_stopped__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<mock_connector_connect_success> net(set, log);
    auto session = std::make_shared<mock_session_manual>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint expected{ "42.42.42.42", 42 };

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    std::promise<std::pair<code, channel::ptr>> connected;
    boost::asio::post(net.strand(), [=, &stopped, &connected]() NOEXCEPT
    {
        session->connect(expected, [&](const code& ec, const channel::ptr& channel) NOEXCEPT
        {
            connected.set_value({ ec, channel });
            return true;
        });

        // connector.connect has been invoked, though its handler is pending.
        // Stop the session after connect but before handle_connect is invoked.
        session->stop();
        stopped.set_value(true);
    });

    const auto result = connected.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::service_stopped);
    BOOST_REQUIRE(!result.second);

    BOOST_REQUIRE(session->require_connected());
    BOOST_REQUIRE_EQUAL(session->start_connect_endpoint(), expected);
    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_manual__handle_channel_start__handshake_error__expected)
{
    const logger log{};
    settings set(selection::mainnet);

    mock_net<mock_connector_connect_success> net(set, log);

    auto session = std::make_shared<mock_session_manual_handshake_failure>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const endpoint expected{ "42.42.42.42", 42 };

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    auto first = true;
    std::promise<std::pair<code, channel::ptr>> connected;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session->connect(expected, [&](const code& ec, const channel::ptr& channel) NOEXCEPT
        {
            if (first)
            {
                connected.set_value({ ec, channel });
                first = false;
            }

            // Continue after connect fail, reenters here.
            return true;
        });
    });

    // mock_session_manual_handshake_failure sets channel.stop(invalid_checksum).
    const auto result = connected.get_future().get();
    BOOST_REQUIRE_EQUAL(result.first, error::invalid_checksum);
    BOOST_REQUIRE(!result.second);

    BOOST_REQUIRE(session->require_connected());
    BOOST_REQUIRE_EQUAL(session->start_connect_endpoint(), expected);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(session->attached_handshake());
}

// start via network (not required for coverage)
// ============================================================================

BOOST_AUTO_TEST_CASE(session_manual__start__network_start__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);

    std::promise<code> started;
    net.start([&](const code& ec) NOEXCEPT
    {
        started.set_value(ec);
    });
    
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_no_connections__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.manual.peers.empty());

    // Connector is not invoked.
    mock_net<> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
        net.run([&](const code& ec) NOEXCEPT
        {
            run.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_configured_connection__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.manual.peers.empty());

    const endpoint expected{ "42.42.42.42", 42 };
    set.manual.peers.push_back(expected);

    // Connect will return invalid_magic when executed.
    mock_net<mock_connector_connect_fail> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
        net.run([&](const code& ec) NOEXCEPT
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
    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_configured_connections__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.manual.peers.empty());

    const endpoint expected{ "42.42.42.4", 42 };
    set.manual.peers.push_back({ "42.42.42.1", 42 });
    set.manual.peers.push_back({ "42.42.42.2", 42 });
    set.manual.peers.push_back({ "42.42.42.3", 42 });
    set.manual.peers.push_back(expected);

    // Connect will return invalid_magic when executed.
    mock_net<mock_connector_connect_fail> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
        net.run([&](const code& ec) NOEXCEPT
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
    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect1__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.manual.peers.empty());

    const endpoint expected{ "42.42.42.42", 42 };

    // Connect will return invalid_magic when executed.
    mock_net<mock_connector_connect_fail> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
        net.run([&](const code& ec) NOEXCEPT
        {
            net.connect(expected);
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
    net.close();
    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect2__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.manual.peers.empty());

    const endpoint expected{ "42.42.42.42", 42 };

    // Connect will return invalid_magic when executed.
    mock_net<mock_connector_connect_fail> net(set, log);

    std::promise<code> start;
    std::promise<code> run;
    net.start([&](const code& ec) NOEXCEPT
    {
        start.set_value(ec);
        net.run([&](const code& ec) NOEXCEPT
        {
            net.connect(expected);
            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);
    net.close();
    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
}

BOOST_AUTO_TEST_CASE(session_manual__start__network_run_connect3__success)
{
    const logger log{};
    settings set(selection::mainnet);
    BOOST_REQUIRE(set.manual.peers.empty());

    const endpoint expected{ "42.42.42.42", 42 };

    // Connect will return invalid_magic when executed, unless service is stopped.
    mock_net<mock_connector_connect_fail> net(set, log);

    auto first = true;
    std::promise<code> start{};
    std::promise<code> run{};
    std::promise<std::pair<code, channel::ptr>> connect{};
    net.start([&](const code& ec)
    {
        start.set_value(ec);
        net.run([&](const code& ec) NOEXCEPT
        {
            net.connect(expected, [&](const code& ec, const channel::ptr& channel) NOEXCEPT
            {
                if (first)
                {
                    connect.set_value({ ec, channel });
                    first = false;
                }

                // Continue after connect fail, reenters here.
                return true;
            });

            run.set_value(ec);
        });
    });

    // Connection failures are logged and suppressed in retry loop.
    BOOST_REQUIRE_EQUAL(start.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(run.get_future().get(), error::success);

    // connector.connect sets invalid_magic, causing a timer reconnect.
    const auto result = connect.get_future().get();

    // The connection loops on connect failure until service stop.
    net.close();

    BOOST_REQUIRE_EQUAL(net.get_connector()->peer(), expected);
    BOOST_REQUIRE_EQUAL(result.first, error::invalid_magic);
    BOOST_REQUIRE(!result.second);
}

// options

class mock_session_manual_options
  : public session_manual
{
public:
    using session_manual::session_manual;
    using session_manual::options;
};

BOOST_AUTO_TEST_CASE(session_manual__options__always__manual_settings)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);
    const auto session = std::make_shared<mock_session_manual_options>(net, 1);
    BOOST_REQUIRE_EQUAL(&session->options(), &net.network_settings().manual);
}

// Manual connection to the test acting as a raw peer on loopback.
// ============================================================================

static constexpr uint16_t manual_peer_port = 65150;
static constexpr uint16_t manual_closed_port = 65151;

struct manual_peer
{
    using configurator = std::function<void(settings&)>;

    manual_peer(const configurator& configure)
      : set_{ selection::mainnet }, net_{ set_, log_ }
    {
        test::clear(test::directory);
        set_.path = TEST_DIRECTORY;
        set_.inbound.connections = 0;
        set_.outbound.connections = 0;
        set_.outbound.seeds.clear();
        configure(set_);

        std::promise<code> started{};
        net_.start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });

        BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

        std::promise<code> running{};
        net_.run([&](const code& ec) NOEXCEPT
        {
            running.set_value(ec);
        });

        BOOST_REQUIRE_EQUAL(running.get_future().get(), error::success);
    }

    ~manual_peer()
    {
        socket_.close();
        net_.close();
    }

    // Start a manual connection, the notifier returns keep.
    std::future<code> connect(uint16_t port, bool keep)
    {
        net_.connect({ "127.0.0.1", port }, [this, keep](const code& ec, const channel::ptr&) NOEXCEPT
        {
            if (!notified_.exchange(true))
                connected_.set_value(ec);

            return keep;
        });

        return connected_.get_future();
    }

    void accept()
    {
        acceptor_.accept(socket_);
    }

    template <class Message>
    void send(const Message& message, uint32_t version)
    {
        using namespace messages::peer;
        system::data_chunk payload(message.size(version));
        BOOST_REQUIRE(message.serialize(version, payload));
        const auto head = heading::factory(set_.identifier, Message::command, payload);
        system::data_chunk frame(heading::size());
        BOOST_REQUIRE(head.serialize({ frame.data(), std::next(frame.data(), heading::size()) }));
        boost::asio::write(socket_, boost::asio::buffer(system::splice(frame, payload)));
    }

    // Read framed messages until the command matches.
    system::data_chunk receive(const std::string& command)
    {
        using namespace messages::peer;
        while (true)
        {
            system::data_array<heading::size()> head_data{};
            boost::asio::read(socket_, boost::asio::buffer(head_data));
            const auto head = heading::deserialize(head_data);
            BOOST_REQUIRE(head);

            system::data_chunk payload(head->payload_size);
            boost::asio::read(socket_, boost::asio::buffer(payload));
            if (head->command == command)
                return payload;
        }
    }

    // Receive the node version, send ours (and sendaddrv2), exchange verack.
    messages::peer::version::cptr handshake(uint32_t value, bool address_v2)
    {
        using namespace messages::peer;
        const auto node = version::deserialize(value, receive(version::command));
        BOOST_REQUIRE(node);

        version out{};
        out.value = value;
        out.services = service::node_none;
        out.timestamp = system::sign_cast<uint64_t>(zulu_time());
        out.nonce = 42424242;
        out.user_agent = "/test/";
        out.start_height = 0;
        out.relay = false;
        send(out, value);

        if (address_v2)
            send(send_address_v2{}, value);

        receive(version_acknowledge::command);
        send(version_acknowledge{}, value);
        return node;
    }

    // Read until the node disconnects, returning the terminal read code.
    boost_code disconnected()
    {
        boost_code ec{};
        uint8_t byte{};
        while (!ec)
            boost::asio::read(socket_, boost::asio::buffer(&byte, one), ec);

        return ec;
    }

private:
    settings set_;
    const logger log_{};
    mock_net<> net_;
    std::atomic_bool notified_{ false };
    std::promise<code> connected_{};
    boost::asio::io_context io_{};
    boost::asio::ip::tcp::acceptor acceptor_{ io_, { boost::asio::ip::address_v4::loopback(), manual_peer_port } };
    boost::asio::ip::tcp::socket socket_{ io_ };
};

BOOST_AUTO_TEST_CASE(session_manual__connect__handshake_70016_address_v2__started_address_requested)
{
    using namespace messages::peer;
    manual_peer peer{ [](settings& set)
    {
        set.enable_address = true;
        set.enable_alert = true;
        set.enable_reject = true;
    } };

    auto connected = peer.connect(manual_peer_port, true);
    peer.accept();
    const auto node = peer.handshake(level::bip155, true);
    BOOST_REQUIRE_EQUAL(node->value, level::bip155);
    BOOST_REQUIRE_EQUAL(connected.get(), error::success);
    BOOST_REQUIRE(get_address::deserialize(level::bip155, peer.receive(get_address::command)));
}

BOOST_AUTO_TEST_CASE(session_manual__connect__handshake_70014_compact__started_address_requested)
{
    using namespace messages::peer;
    manual_peer peer{ [](settings& set)
    {
        set.protocol_maximum = level::bip152;
        set.enable_compact = true;
        set.enable_address = true;
    } };

    auto connected = peer.connect(manual_peer_port, true);
    peer.accept();
    const auto node = peer.handshake(level::bip152, false);
    BOOST_REQUIRE_EQUAL(node->value, level::bip152);
    BOOST_REQUIRE_EQUAL(connected.get(), error::success);
    BOOST_REQUIRE(get_address::deserialize(level::bip152, peer.receive(get_address::command)));
}

BOOST_AUTO_TEST_CASE(session_manual__connect__handshake_70002_reject__started)
{
    using namespace messages::peer;
    manual_peer peer{ [](settings& set)
    {
        set.protocol_maximum = level::bip61;
        set.enable_reject = true;
    } };

    auto connected = peer.connect(manual_peer_port, true);
    peer.accept();
    const auto node = peer.handshake(level::bip61, false);
    BOOST_REQUIRE_EQUAL(node->value, level::bip61);
    BOOST_REQUIRE_EQUAL(connected.get(), error::success);
}

BOOST_AUTO_TEST_CASE(session_manual__connect__handshake_70001__started)
{
    using namespace messages::peer;
    manual_peer peer{ [](settings& set)
    {
        set.protocol_maximum = level::bip37;
    } };

    auto connected = peer.connect(manual_peer_port, true);
    peer.accept();
    const auto node = peer.handshake(level::bip37, false);
    BOOST_REQUIRE_EQUAL(node->value, level::bip37);
    BOOST_REQUIRE_EQUAL(connected.get(), error::success);
}

BOOST_AUTO_TEST_CASE(session_manual__connect__handshake_31800_address__started_address_requested)
{
    using namespace messages::peer;
    manual_peer peer{ [](settings& set)
    {
        set.protocol_maximum = level::headers_protocol;
        set.enable_address = true;
    } };

    auto connected = peer.connect(manual_peer_port, true);
    peer.accept();
    const auto node = peer.handshake(level::headers_protocol, false);
    BOOST_REQUIRE_EQUAL(node->value, level::headers_protocol);
    BOOST_REQUIRE_EQUAL(connected.get(), error::success);
    BOOST_REQUIRE(get_address::deserialize(level::headers_protocol, peer.receive(get_address::command)));
}

BOOST_AUTO_TEST_CASE(session_manual__connect__notifier_drops_started__disconnected)
{
    using namespace messages::peer;
    manual_peer peer{ [](settings&) {} };

    auto connected = peer.connect(manual_peer_port, false);
    peer.accept();
    peer.handshake(level::bip155, false);
    BOOST_REQUIRE_EQUAL(connected.get(), error::success);
    const auto ec = peer.disconnected();
    BOOST_REQUIRE(ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset);
}

BOOST_AUTO_TEST_CASE(session_manual__connect__notifier_drops_connect_failure__failure)
{
    manual_peer peer{ [](settings& set)
    {
        set.connect_timeout_seconds = 1;
    } };

    auto connected = peer.connect(manual_closed_port, false);
    BOOST_REQUIRE(connected.get());
}

BC_POP_WARNING()

BOOST_AUTO_TEST_SUITE_END()
