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

struct session_outbound_tests_setup_fixture
{
    session_outbound_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }

    ~session_outbound_tests_setup_fixture()
    {
        test::remove(TEST_NAME);
    }
};

BOOST_FIXTURE_TEST_SUITE(session_outbound_tests, session_outbound_tests_setup_fixture)

using namespace network::messages::peer;
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

    // Get captured hostname.
    std::string hostname() const NOEXCEPT
    {
        return hostname_;
    }

    // Get captured port.
    uint16_t port() const NOEXCEPT
    {
        return port_;
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
    void start(const std::string& hostname, uint16_t port,
        const config::address&, const config::endpoint&,
        socket_handler&& handler) NOEXCEPT override
    {
        if (is_zero(connects_++))
        {
            hostname_ = hostname;
            port_ = port;
        }

        socket::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
        const auto socket = std::make_shared<network::socket>(log, service_, std::move(params));

        // Must be asynchronous or is an infinite recursion.
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            // Connect result code is independent of the channel stop code.
            // As error code would set the re-listener timer, channel pointer is ignored.
            handler(error::success, socket);
        });
    }

protected:
    bool stopped_{ false };
    size_t connects_{ zero };
    std::string hostname_;
    uint16_t port_;
};

class mock_connector_connect_fail
  : public connector
{
public:
    typedef std::shared_ptr<mock_connector_connect_fail> ptr;

    using connector::connector;

    void start(const std::string&, uint16_t, const config::address&,
        const config::endpoint&, socket_handler&& handler) NOEXCEPT override
    {
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            handler(error::invalid_magic, nullptr);
        });
    }
};

class mock_session_outbound
  : public session_outbound
{
public:
    using session_outbound::session_outbound;

    bool stopped() const NOEXCEPT override
    {
        return session_outbound::stopped();
    }

    // Capture first start_connect call.
    void start_connect(const code&, size_t slot) NOEXCEPT override
    {
        // Must be first to ensure connector::start_connect() preceeds promise release.
        session_outbound::start_connect({}, slot);

        if (is_one(connects_))
            reconnect_.set_value(true);

        if (is_zero(connects_++))
            connect_.set_value(true);
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
        // Outbound channels handshake concurrently (one strand each).
        if (!handshaked_.exchange(true))
            handshake_.set_value(true);

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
    mutable std::atomic_bool handshaked_{ false };
    mutable std::promise<bool> handshake_;

private:
    code connect_code_{ error::success };
    size_t connects_{ zero };
    mutable std::promise<bool> connect_;
    mutable std::promise<bool> reconnect_;
};

class mock_session_outbound_one_address_count
  : public mock_session_outbound
{
public:
    using mock_session_outbound::mock_session_outbound;

    size_t address_count() const NOEXCEPT override
    {
        return 1;
    }
};

class mock_session_outbound_one_address
  : public mock_session_outbound_one_address_count
{
public:
    typedef std::shared_ptr<mock_session_outbound_one_address> ptr;

    using mock_session_outbound_one_address_count::
        mock_session_outbound_one_address_count;

    void take(hosts::family, address_item_handler&& handler) const NOEXCEPT override
    {
        // Default address is ipv6, will case disabled(address) true.
        handler(error::success, system::to_shared<const address_item>());
    }
};

class mock_session_outbound_no_address
  : public mock_session_outbound_one_address_count
{
public:
    typedef std::shared_ptr<mock_session_outbound_no_address> ptr;

    using mock_session_outbound_one_address_count::
        mock_session_outbound_one_address_count;

    void take(hosts::family, address_item_handler&& handler) const NOEXCEPT override
    {
        handler(error::address_not_found, {});
    }

    void seed() const NOEXCEPT override
    {
        seeded_ = true;
    }

    bool seeded() const NOEXCEPT
    {
        return seeded_;
    }

private:
    mutable std::atomic_bool seeded_{ false };
};

class mock_session_outbound_groups
  : public mock_session_outbound_one_address
{
public:
    typedef std::shared_ptr<mock_session_outbound_groups> ptr;

    using mock_session_outbound_one_address::mock_session_outbound_one_address;

    // Capture the slot of each start_connect call.
    void start_connect(const code& ec, size_t slot) NOEXCEPT override
    {
        slots_.push_back(slot);
        mock_session_outbound_one_address::start_connect(ec, slot);
    }

    const std::vector<size_t>& slots() const NOEXCEPT
    {
        return slots_;
    }

private:
    std::vector<size_t> slots_{};
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
        return attach<mock_session_outbound>(*this);
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

    class mock_session_outbound
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

class mock_connector_stop_connect
  : public mock_connector_connect_success
{
public:
    typedef std::shared_ptr<mock_connector_stop_connect> ptr;

    mock_connector_stop_connect(const logger& log, asio::strand& strand,
        asio::context& service, connector::parameters&& params,
        mock_session_outbound::ptr session) NOEXCEPT
      : mock_connector_connect_success(log, strand, service, suspended_,
          std::move(params)),
        session_(session)
    {
    }

    void start(const std::string& hostname, uint16_t port,
        const config::address& address, const config::endpoint& endpoint,
        socket_handler&& handler) NOEXCEPT override
    {
        BC_ASSERT_MSG(session_, "call set_session");

        // This connector.start_connect is invoked from network stranded method.
        session_->stop();

        mock_connector_connect_success::start(hostname, port, address,
            endpoint, std::move(handler));
    }

private:
    mock_session_outbound::ptr session_;
    std::atomic_bool suspended_{ false };
};

// Can't derive from mock_net because Connector has more arguments.
class mock_net_stop_connect
  : public net
{
public:
    using net::net;

    void set_session(mock_session_outbound::ptr session) NOEXCEPT
    {
        session_ = session;
    }

    // Get first created connector.
    mock_connector_stop_connect::ptr get_connector() const NOEXCEPT
    {
        return connector_;
    }

    // Create mock connector to inject mock channel.
    connector::ptr to_connector(const settings::socks5& ,
        const settings::tcp_server& options,
        const steady_clock::duration& timeout, size_t) NOEXCEPT override
    {
        if (connector_)
            return connector_;

        connector::parameters params
        {
            .connect_timeout = timeout,
            .maximum_request = options.maximum_request
        };

        return ((connector_ = std::make_shared<mock_connector_stop_connect>(
            log, strand(), service(), std::move(params), session_)));
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
    mock_connector_stop_connect::ptr connector_;
    mock_session_outbound::ptr session_;

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

BOOST_AUTO_TEST_CASE(session_outbound__stop__started__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.connections = 1;
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_outbound_one_address_count>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    // This indicates successful start, not connection(s) status.
    // Because net is not started, connections will fail until stop.
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

BOOST_AUTO_TEST_CASE(session_outbound__stop__stopped__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);
    mock_session_outbound session(net, 1);

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

BOOST_AUTO_TEST_CASE(session_outbound__start__no_outbound_connections__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 0;
    set.outbound.host_pool_capacity = 1;
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_outbound_one_address_count>(net, 1);
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
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_outbound__start__no_host_pool_capacity__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_outbound_one_address_count>(net, 1);
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
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_outbound__start__zero_connect_batch_size__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 0;
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_outbound_one_address_count>(net, 1);
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
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_outbound__start__no_address_count__address_not_found)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_outbound>(net, 1);
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

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::address_not_found);
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_outbound__start__no_address__seeded)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.connections = 1;
    set.outbound.connect_timeout_seconds = 10000;
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_outbound_no_address>(net, 1);
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

    std::promise<bool> seed;
    boost::asio::post(net.strand(), [=, &seed]() NOEXCEPT
    {
        seed.set_value(session->seeded());
    });

    const auto seeded = seed.get_future().get();

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(seeded);
}

BOOST_AUTO_TEST_CASE(session_outbound__start__restart__operation_failed)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.connections = 1;
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_outbound_one_address_count>(net, 1);
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

    std::promise<code> restarted;
    boost::asio::post(net.strand(), [=, &restarted]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            restarted.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(restarted.get_future().get(), error::operation_failed);
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

// Connection errors get eaten with all connect failure codes (logging only).
BOOST_AUTO_TEST_CASE(session_outbound__start__three_outbound_three_batch__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 3;
    set.outbound.connections = 3;
    set.outbound.connect_timeout_seconds = 10000;
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_outbound_one_address>(net, 1);
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

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

// Socket termination (sockets have no stop codes).

BOOST_AUTO_TEST_CASE(session_outbound__start__handle_connect_stopped__first_channel_service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 2;
    set.outbound.connections = 2;
    set.outbound.connect_timeout_seconds = 10000;

    // Prevent default address from being rejected by gossip_ipv6 false.
    set.gossip_ipv6 = true;

    // This invokes session.stop from within start_connect and then continues.
    // First channel is stopped for service_stopped and others for channel_dropped.
    mock_net_stop_connect net(set, log);
    auto session = std::make_shared<mock_session_outbound_one_address>(net, 1);
    net.set_session(session);
    BOOST_REQUIRE(session->stopped());

    // Started session calls session.stop upon first connect.
    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_outbound__start__handle_one__first_channel_success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.connections = 1;
    set.outbound.connect_timeout_seconds = 10000;

    // Prevent default address from being rejected by gossip_ipv6 false.
    set.gossip_ipv6 = true;

    // Started channel results in read failure.
    mock_net<mock_connector_connect_success> net(set, log);
    auto session = std::make_shared<mock_session_outbound_one_address>(net, 1);
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

    // Block until connected.
    BOOST_REQUIRE(session->require_connected());
    BOOST_REQUIRE(session->require_attached_handshake());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

// set_connections

BOOST_AUTO_TEST_CASE(session_outbound__set_connections__stopped__applied_at_start)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.connections = 3;
    set.outbound.connect_timeout_seconds = 10000;
    set.retry_timeout_seconds = 10000;
    set.gossip_ipv6 = true;
    mock_net<mock_connector_connect_fail> net(set, log);
    auto session = std::make_shared<mock_session_outbound_groups>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<size_t> configured;
    boost::asio::post(net.strand(), [=, &configured]() NOEXCEPT
    {
        configured.set_value(session->connections());
    });

    BOOST_REQUIRE_EQUAL(configured.get_future().get(), 3u);

    std::promise<size_t> reduced;
    boost::asio::post(net.strand(), [=, &reduced]() NOEXCEPT
    {
        session->set_connections(1);
        reduced.set_value(session->connections());
    });

    BOOST_REQUIRE_EQUAL(reduced.get_future().get(), 1u);
    BOOST_REQUIRE(session->slots().empty());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(session->slots().size(), 1u);
    BOOST_REQUIRE_EQUAL(session->slots().at(0), 0u);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_outbound__set_connections__started_raised__slots_started)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.connections = 1;
    set.outbound.connect_timeout_seconds = 10000;
    set.retry_timeout_seconds = 10000;
    set.gossip_ipv6 = true;
    mock_net<mock_connector_connect_fail> net(set, log);
    auto session = std::make_shared<mock_session_outbound_groups>(net, 1);
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
    BOOST_REQUIRE_EQUAL(session->slots().size(), 1u);

    std::promise<size_t> raised;
    boost::asio::post(net.strand(), [=, &raised]() NOEXCEPT
    {
        session->set_connections(3);
        raised.set_value(session->connections());
    });

    BOOST_REQUIRE_EQUAL(raised.get_future().get(), 3u);
    BOOST_REQUIRE_EQUAL(session->slots().size(), 3u);
    BOOST_REQUIRE_EQUAL(session->slots().at(0), 0u);
    BOOST_REQUIRE_EQUAL(session->slots().at(1), 1u);
    BOOST_REQUIRE_EQUAL(session->slots().at(2), 2u);

    std::promise<size_t> unchanged;
    boost::asio::post(net.strand(), [=, &unchanged]() NOEXCEPT
    {
        session->set_connections(3);
        unchanged.set_value(session->connections());
    });

    BOOST_REQUIRE_EQUAL(unchanged.get_future().get(), 3u);
    BOOST_REQUIRE_EQUAL(session->slots().size(), 3u);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_outbound__set_connections__started_lowered__slot_ended_on_retry)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.host_pool_capacity = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.connections = 1;
    set.outbound.connect_timeout_seconds = 10000;
    set.retry_timeout_seconds = 1;
    set.gossip_ipv6 = true;
    mock_net<mock_connector_connect_fail> net(set, log);
    auto session = std::make_shared<mock_session_outbound_groups>(net, 1);
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
    BOOST_REQUIRE_EQUAL(session->slots().size(), 1u);

    std::promise<size_t> lowered;
    boost::asio::post(net.strand(), [=, &lowered]() NOEXCEPT
    {
        session->set_connections(0);
        lowered.set_value(session->connections());
    });

    // The connect failure retry reenters start_connect, which ends the slot.
    BOOST_REQUIRE_EQUAL(lowered.get_future().get(), 0u);
    BOOST_REQUIRE(session->require_reconnect());
    BOOST_REQUIRE_EQUAL(session->slots().size(), 2u);
    BOOST_REQUIRE_EQUAL(session->slots().at(1), 0u);

    std::promise<size_t> raised;
    boost::asio::post(net.strand(), [=, &raised]() NOEXCEPT
    {
        session->set_connections(1);
        raised.set_value(session->connections());
    });

    // The slot is restarted.
    BOOST_REQUIRE_EQUAL(raised.get_future().get(), 1u);
    BOOST_REQUIRE_EQUAL(session->slots().size(), 3u);
    BOOST_REQUIRE_EQUAL(session->slots().at(2), 0u);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

// Outbound connection to the test acting as a raw peer on loopback.
// ============================================================================

static constexpr uint16_t outbound_peer_port = 65154;
static constexpr uint16_t outbound_closed_port = 65155;

// A network with one pooled address (the given port) that restores nothing.
class outbound_net
  : public net
{
public:
    outbound_net(const settings& set, const logger& log, const code& take_code, uint16_t port) NOEXCEPT
      : net(set, log), take_code_(take_code), port_(port)
    {
    }

    size_t address_count() const NOEXCEPT override
    {
        return one;
    }

    void take(hosts::family, address_item_handler&& handler) NOEXCEPT override
    {
        if (is_one(takes_++))
            retaken_.set_value(true);

        const auto timestamp = system::possible_narrow_cast<uint32_t>(unix_time());
        const auto item = config::address{ "127.0.0.1:" + std::to_string(port_) }.to_address_item(timestamp, service::node_none);
        handler(take_code_, system::to_shared(item));
    }

    void restore(const address_item_cptr&, result_handler&& handler) NOEXCEPT override
    {
        handler(error::success);
    }

    bool retaken() const NOEXCEPT
    {
        return retaken_.get_future().get();
    }

private:
    const code take_code_;
    const uint16_t port_;
    size_t takes_{};
    mutable std::promise<bool> retaken_{};
};

struct outbound_peer
{
    using configurator = std::function<void(settings&)>;

    outbound_peer(const configurator& configure, const code& take_code, uint16_t port)
      : set_{ selection::mainnet }, net_{ set_, log_, take_code, port }
    {
        test::clear(test::directory);
        set_.path = TEST_DIRECTORY;
        set_.inbound.connections = 0;
        set_.outbound.connections = 1;
        set_.outbound.connect_batch_size = 1;
        set_.outbound.host_pool_capacity = 10;
        set_.outbound.seeds.clear();
        configure(set_);

        std::promise<code> started{};
        net_.start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });

        BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    }

    ~outbound_peer()
    {
        socket_.close();
        net_.close();
    }

    void run()
    {
        std::promise<code> running{};
        net_.run([&](const code& ec) NOEXCEPT
        {
            running.set_value(ec);
        });

        BOOST_REQUIRE_EQUAL(running.get_future().get(), error::success);
    }

    void suspend()
    {
        net_.suspend(error::service_suspended);
    }

    void accept()
    {
        acceptor_.accept(socket_);
    }

    void close()
    {
        socket_.close();
    }

    bool retaken() const
    {
        return net_.retaken();
    }

    template <class Message>
    void send(const Message& message, uint32_t version)
    {
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

    // Receive the node version, send ours, exchange verack.
    version::cptr handshake(uint32_t value)
    {
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

        receive(version_acknowledge::command);
        send(version_acknowledge{}, value);
        return node;
    }

private:
    settings set_;
    const logger log_{};
    outbound_net net_;
    boost::asio::io_context io_{};
    boost::asio::ip::tcp::acceptor acceptor_{ io_, { boost::asio::ip::address_v4::loopback(), outbound_peer_port } };
    boost::asio::ip::tcp::socket socket_{ io_ };
};

BOOST_AUTO_TEST_CASE(session_outbound__attach_protocols__loopback_peer__address_requested_then_reconnected)
{
    outbound_peer peer{ [](settings& set)
    {
        set.enable_address = true;
    }, error::success, outbound_peer_port };

    peer.run();
    peer.accept();
    const auto node = peer.handshake(level::bip155);
    BOOST_REQUIRE_EQUAL(node->value, level::bip155);
    BOOST_REQUIRE(get_address::deserialize(level::bip155, peer.receive(get_address::command)));

    peer.close();
    peer.accept();
    BOOST_REQUIRE(version::deserialize(level::bip155, peer.receive(version::command)));
}

BOOST_AUTO_TEST_CASE(session_outbound__handle_connect__address_not_found__retaken)
{
    outbound_peer peer{ [](settings& set)
    {
        set.outbound.connect_timeout_seconds = 1;
    }, error::address_not_found, outbound_peer_port };

    peer.run();
    BOOST_REQUIRE(peer.retaken());
}

BOOST_AUTO_TEST_CASE(session_outbound__handle_connect__connect_refused__retaken)
{
    outbound_peer peer{ [](settings& set)
    {
        set.outbound.connect_timeout_seconds = 1;
    }, error::success, outbound_closed_port };

    peer.run();
    BOOST_REQUIRE(peer.retaken());
}

BOOST_AUTO_TEST_CASE(session_outbound__handle_channel_stop__full_host_pool__reconnected)
{
    outbound_peer peer{ [](settings& set)
    {
        set.outbound.host_pool_capacity = 1;
    }, error::success, outbound_peer_port };

    peer.run();
    peer.accept();
    peer.handshake(level::bip155);
    peer.close();
    peer.accept();
    BOOST_REQUIRE(version::deserialize(level::bip155, peer.receive(version::command)));
}

BOOST_AUTO_TEST_CASE(session_outbound__handle_one__suspended__retaken)
{
    outbound_peer peer{ [](settings&) {}, error::success, outbound_peer_port };

    peer.suspend();
    peer.run();
    BOOST_REQUIRE(peer.retaken());
}

BOOST_AUTO_TEST_CASE(session_outbound__start_connect__proxied_unreachable__retaken)
{
    outbound_peer peer{ [](settings& set)
    {
        set.outbound.socks = { "127.0.0.1:65157" };
        set.outbound.connect_timeout_seconds = 1;
    }, error::success, outbound_peer_port };

    peer.run();
    BOOST_REQUIRE(peer.retaken());
}

BOOST_AUTO_TEST_SUITE_END()
