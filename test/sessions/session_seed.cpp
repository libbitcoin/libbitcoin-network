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

BOOST_AUTO_TEST_SUITE(session_seed_tests)

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

    // Get captured connection count.
    size_t connects() const NOEXCEPT
    {
        return connects_;
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
  : public mock_connector_connect_success
{
public:
    typedef std::shared_ptr<mock_connector_connect_fail> ptr;

    using mock_connector_connect_success::mock_connector_connect_success;

    void start(const std::string&, uint16_t, const config::address&,
        const config::endpoint&, socket_handler&& handler) NOEXCEPT override
    {
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            handler(error::invalid_magic, nullptr);
        });
    }
};

class mock_connector_connect_pending
  : public mock_connector_connect_success
{
public:
    typedef std::shared_ptr<mock_connector_connect_pending> ptr;

    using mock_connector_connect_success::mock_connector_connect_success;

    void start(const std::string&, uint16_t, const config::address&,
        const config::endpoint&, socket_handler&& handler) NOEXCEPT override
    {
        handler_ = std::move(handler);
    }

    // The pending connect is canceled upon stop.
    void stop() NOEXCEPT override
    {
        if (handler_)
        {
            const auto handler = std::move(handler_);
            handler_ = {};
            boost::asio::post(strand_, [=]() NOEXCEPT
            {
                handler(error::operation_canceled, nullptr);
            });
        }

        mock_connector_connect_success::stop();
    }

private:
    socket_handler handler_{};
};

class mock_connector_connect_suspended
  : public mock_connector_connect_success
{
public:
    typedef std::shared_ptr<mock_connector_connect_suspended> ptr;

    using mock_connector_connect_success::mock_connector_connect_success;

    // A suspended connector invokes its handler without posting.
    void start(const std::string&, uint16_t, const config::address&,
        const config::endpoint&, socket_handler&& handler) NOEXCEPT override
    {
        handler(error::service_suspended, nullptr);
    }
};

class mock_session_seed
  : public session_seed
{
public:
    using session_seed::session_seed;

    bool stopped() const NOEXCEPT override
    {
        return session_seed::stopped();
    }

    // Capture first start_connect call.
    void start_seed(const code&, const config::endpoint& seed,
        const connector::ptr& connector,
        const socket_handler& handler) NOEXCEPT override
    {
        // Must be first to ensure connector::start_connect() preceeds promise release.
        session_seed::start_seed({}, seed, connector, handler);

        if (!seeded_.exchange(true))
            seed_.set_value(true);
    }

    bool seeded() const NOEXCEPT
    {
        return seeded_;
    }

    bool require_seeded() const NOEXCEPT
    {
        return seed_.get_future().get();
    }

    void attach_handshake(const channel::ptr& channel,
        result_handler&& handshake) NOEXCEPT override
    {
        // Seed channels handshake concurrently (one strand each).
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

private:
    mutable std::atomic_bool seeded_{ false };
    mutable std::atomic_bool handshaked_{ false };
    mutable std::promise<bool> seed_;
    mutable std::promise<bool> handshake_;
};

class mock_session_seed_one_address_count
  : public mock_session_seed
{
public:
    using mock_session_seed::mock_session_seed;

    size_t address_count() const NOEXCEPT override
    {
        return 1;
    }
};

class mock_session_seed_increasing_address_count
  : public mock_session_seed
{
public:
    using mock_session_seed::mock_session_seed;

    // Rest to zero on start for restart testing.
    void start(result_handler&& handler) NOEXCEPT override
    {
        count_ = zero;
        mock_session_seed::start(std::move(handler));
    }

    size_t address_count() const NOEXCEPT override
    {
        return count_++;
    }

private:
    mutable size_t count_{ zero };
};

class mock_session_seed_first_fails
  : public mock_session_seed_increasing_address_count
{
public:
    using mock_session_seed_increasing_address_count::
        mock_session_seed_increasing_address_count;

    // Fail the first seed, so the race is sufficient as the others connect.
    void start_seed(const code& ec, const config::endpoint& seed,
        const connector::ptr& connector,
        const socket_handler& handler) NOEXCEPT override
    {
        if (!failed_.exchange(true))
        {
            handler(error::invalid_magic, nullptr);
            return;
        }

        mock_session_seed_increasing_address_count::start_seed(ec, seed,
            connector, handler);
    }

private:
    std::atomic_bool failed_{ false };
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

    code count_channel(const channel_peer&) NOEXCEPT override
    {
        return error::success;
    }

    void uncount_channel(const channel_peer&) NOEXCEPT override
    {
    }

    void save(const messages::peer::address::cptr& message,
        count_handler&& complete) NOEXCEPT override
    {
        hosts_ += message->addresses.size();
        complete(error::success, zero);
    }

    size_t address_count() const NOEXCEPT override
    {
        return hosts_;
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

    size_t hosts_{};
};

class mock_connector_stop_connect
  : public mock_connector_connect_success
{
public:
    typedef std::shared_ptr<mock_connector_stop_connect> ptr;

    mock_connector_stop_connect(const logger& log, asio::strand& strand,
        asio::context& service, connector::parameters&& params,
        mock_session_seed::ptr session) NOEXCEPT
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
    mock_session_seed::ptr session_;
    std::atomic_bool suspended_{ false };
};

// Can't derive from mock_net because Connector has more arguments.
class mock_net_stop_connect
  : public net
{
public:
    using net::net;

    void set_session(mock_session_seed::ptr session) NOEXCEPT
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
    mock_session_seed::ptr session_;

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

BOOST_AUTO_TEST_CASE(session_seed__stop__started_sufficient__expected)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.host_pool_capacity = 1;
    mock_net_stop_connect net(set, log);
    auto session = std::make_shared<mock_session_seed_increasing_address_count>(net, 1);
    net.set_session(session);
    BOOST_REQUIRE(session->stopped());
    
    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });

        session->stop();
    });

    // This is a race between success and seeding_unsuccessful.
    // This tends toward success with HAVE_LOGGING and otherwise without.
    const auto ec = started.get_future().get();
    BOOST_REQUIRE(ec == error::success || ec == error::seeding_unsuccessful);
    BOOST_REQUIRE_EQUAL(net.get_connector()->connects(), 1u);
    BOOST_REQUIRE(!session->attached_handshake());
    BOOST_REQUIRE(session->stopped());

    // Block until started connectors/channels complete before clearing session.
    net.close();
}

BOOST_AUTO_TEST_CASE(session_seed__stop__stopped__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_net<> net(set, log);
    mock_session_seed session(net, 1);
    BOOST_REQUIRE(session.stopped());

    std::promise<bool> promise;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session.stop();
        promise.set_value(true);
    });

    BOOST_REQUIRE(promise.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

//start

BOOST_AUTO_TEST_CASE(session_seed__start__no_outbound__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 0;
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
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

BOOST_AUTO_TEST_CASE(session_seed__start__outbound_one_address_count__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.host_pool_capacity = 1;
    BOOST_REQUIRE_EQUAL(set.outbound.minimum_address_count(), one);

    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_seed_one_address_count>(net, 1);
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

BOOST_AUTO_TEST_CASE(session_seed__start__outbound_no_host_pool_capacity__seeding_unsuccessful)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.host_pool_capacity = 0;
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_seed__start__outbound_no_seeds__seeding_unsuccessful)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.host_pool_capacity = 1;
    set.outbound.seeds.clear();
    mock_net<> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_seed__start__restart__operation_failed)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.host_pool_capacity = 1;
    set.outbound.seeds.resize(3);
    BOOST_REQUIRE_EQUAL(set.outbound.minimum_address_count(), one);

    mock_net<mock_connector_connect_fail> net(set, log);
    auto session = std::make_shared<mock_session_seed_increasing_address_count>(net, 1);
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

BOOST_AUTO_TEST_CASE(session_seed__start__seeded__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.host_pool_capacity = 1;
    set.outbound.seeds.resize(2);
    BOOST_REQUIRE_EQUAL(set.outbound.minimum_address_count(), one);

    mock_net<mock_connector_connect_success> net(set, log);
    auto session = std::make_shared<mock_session_seed_increasing_address_count>(net, 1);
    BOOST_REQUIRE(session->stopped());
    
    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(!session->stopped());

    // No need to block since seeding completes at started true.
    BOOST_REQUIRE(net.get_connector()->connected_());
    BOOST_REQUIRE(session->attached_handshake());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_seed__start__not_seeded__seeding_unsuccessful)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.host_pool_capacity = 1;
    mock_net<mock_connector_connect_success> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
    BOOST_REQUIRE(session->stopped());
    
    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(net.get_connector()->connected_());
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

// seeding

BOOST_AUTO_TEST_CASE(session_seed__seeding__not_started__false)
{
    const logger log{};
    const settings set(selection::mainnet);
    mock_net<mock_connector_connect_success> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<bool> seeding;
    boost::asio::post(net.strand(), [=, &seeding]() NOEXCEPT
    {
        seeding.set_value(session->seeding());
    });

    BOOST_REQUIRE(!seeding.get_future().get());
}

BOOST_AUTO_TEST_CASE(session_seed__seeding__no_outbound__false)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 0;
    mock_net<mock_connector_connect_success> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
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

    std::promise<bool> seeding;
    boost::asio::post(net.strand(), [=, &seeding]() NOEXCEPT
    {
        seeding.set_value(session->seeding());
    });

    BOOST_REQUIRE(!seeding.get_future().get());
}

BOOST_AUTO_TEST_CASE(session_seed__seeding__connect_fail__false)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.host_pool_capacity = 1;
    mock_net<mock_connector_connect_fail> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);

    std::promise<bool> seeding;
    boost::asio::post(net.strand(), [=, &seeding]() NOEXCEPT
    {
        seeding.set_value(session->seeding());
    });

    BOOST_REQUIRE(!seeding.get_future().get());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

BOOST_AUTO_TEST_CASE(session_seed__seeding__connecting__true)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.host_pool_capacity = 1;
    mock_net<mock_connector_connect_pending> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    std::promise<bool> seeding;
    boost::asio::post(net.strand(), [=, &seeding]() NOEXCEPT
    {
        seeding.set_value(session->seeding());
    });

    const auto connecting = seeding.get_future().get();

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);
    BOOST_REQUIRE(connecting);
}

BOOST_AUTO_TEST_CASE(session_seed__seeding__connecting_stopped__false)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.host_pool_capacity = 1;
    mock_net<mock_connector_connect_pending> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);

    std::promise<bool> seeding;
    boost::asio::post(net.strand(), [=, &seeding]() NOEXCEPT
    {
        seeding.set_value(session->seeding());
    });

    BOOST_REQUIRE(!seeding.get_future().get());
}

BOOST_AUTO_TEST_CASE(session_seed__seeding__sufficient_connecting__true)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.connect_batch_size = 1;
    set.outbound.host_pool_capacity = 1;
    set.outbound.seeds.resize(2);
    BOOST_REQUIRE_EQUAL(set.outbound.minimum_address_count(), one);
    mock_net<mock_connector_connect_pending> net(set, log);
    auto session = std::make_shared<mock_session_seed_first_fails>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    std::promise<bool> seeding;
    boost::asio::post(net.strand(), [=, &seeding]() NOEXCEPT
    {
        seeding.set_value(session->seeding());
    });

    const auto connecting = seeding.get_future().get();

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(connecting);
}

BOOST_AUTO_TEST_CASE(session_seed__seeding__connect_suspended__false)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound.connections = 1;
    set.outbound.host_pool_capacity = 1;
    mock_net<mock_connector_connect_suspended> net(set, log);
    auto session = std::make_shared<mock_session_seed>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);

    std::promise<bool> seeding;
    boost::asio::post(net.strand(), [=, &seeding]() NOEXCEPT
    {
        seeding.set_value(session->seeding());
    });

    BOOST_REQUIRE(!seeding.get_future().get());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
}

////BOOST_AUTO_TEST_CASE(session_seed__live__one_address__expected)
////{
////    const logger log{};
////    settings set(selection::mainnet);
////    set.outbound.seeds.resize(1);
////    set.seeding_timeout_seconds = 5;
////    set.outbound.connections = 1;
////    set.outbound.host_pool_capacity = 1;
////    mock_net<> net(set, log);
////    auto session = std::make_shared<session_seed>(net, 1);
////
////    std::promise<code> started;
////    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
////    {
////        session->start([&](const code& ec) NOEXCEPT
////        {
////            started.set_value(ec);
////        });
////    });
////
////    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
////
////    std::promise<bool> stopped;
////    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
////    {
////        session->stop();
////        stopped.set_value(true);
////    });
////
////    BOOST_REQUIRE(stopped.get_future().get());
////    BOOST_REQUIRE_GT(net.address_count(), zero);
////}

// options

class mock_session_seed_options
  : public session_seed
{
public:
    using session_seed::session_seed;
    using session_seed::options;
};

BOOST_AUTO_TEST_CASE(session_seed__options__always__outbound_settings)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_session_seed_options>(instance, 1);
    BOOST_REQUIRE_EQUAL(&session->options(), &instance.network_settings().outbound);
}

// Seed connection to the test acting as a raw peer on loopback.
// ============================================================================

static constexpr uint16_t seed_peer_port = 65153;

struct seed_peer
{
    using configurator = std::function<void(settings&)>;

    seed_peer(const configurator& configure)
      : set_{ selection::mainnet }, net_{ set_, log_ }
    {
        test::clear(test::directory);
        set_.path = TEST_DIRECTORY;
        set_.inbound.connections = 0;
        set_.outbound.connections = 1;
        set_.outbound.connect_batch_size = 1;
        set_.outbound.host_pool_capacity = 1;
        set_.outbound.seeds.clear();
        set_.outbound.seeds.emplace_back("127.0.0.1", seed_peer_port);
        configure(set_);

        net_.start([this](const code& ec) NOEXCEPT
        {
            started_.set_value(ec);
        });

        acceptor_.accept(socket_);
    }

    ~seed_peer()
    {
        socket_.close();
        net_.close();
    }

    code started()
    {
        return started_.get_future().get();
    }

    void close()
    {
        socket_.close();
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
    net net_;
    std::promise<code> started_{};
    boost::asio::io_context io_{};
    boost::asio::ip::tcp::acceptor acceptor_{ io_, { boost::asio::ip::address_v4::loopback(), seed_peer_port } };
    boost::asio::ip::tcp::socket socket_{ io_ };
};

BOOST_AUTO_TEST_CASE(session_seed__attach_handshake__70016_alert_reject__address_requested)
{
    seed_peer peer{ [](settings& set)
    {
        set.enable_alert = true;
        set.enable_reject = true;
    } };

    const auto node = peer.handshake(level::bip155);
    BOOST_REQUIRE_EQUAL(node->value, level::bip155);
    BOOST_REQUIRE(!node->relay);
    BOOST_REQUIRE(get_address::deserialize(level::bip155, peer.receive(get_address::command)));
    peer.close();
    BOOST_REQUIRE_EQUAL(peer.started(), error::seeding_unsuccessful);
}

BOOST_AUTO_TEST_CASE(session_seed__attach_handshake__70002_reject__address_requested)
{
    seed_peer peer{ [](settings& set)
    {
        set.protocol_maximum = level::bip61;
        set.enable_reject = true;
    } };

    const auto node = peer.handshake(level::bip61);
    BOOST_REQUIRE_EQUAL(node->value, level::bip61);
    BOOST_REQUIRE(!node->relay);
    BOOST_REQUIRE(get_address::deserialize(level::bip61, peer.receive(get_address::command)));
    peer.close();
    BOOST_REQUIRE_EQUAL(peer.started(), error::seeding_unsuccessful);
}

BOOST_AUTO_TEST_CASE(session_seed__attach_handshake__70001__address_requested)
{
    seed_peer peer{ [](settings& set)
    {
        set.protocol_maximum = level::bip37;
    } };

    const auto node = peer.handshake(level::bip37);
    BOOST_REQUIRE_EQUAL(node->value, level::bip37);
    BOOST_REQUIRE(!node->relay);
    BOOST_REQUIRE(get_address::deserialize(level::bip37, peer.receive(get_address::command)));
    peer.close();
    BOOST_REQUIRE_EQUAL(peer.started(), error::seeding_unsuccessful);
}

BOOST_AUTO_TEST_CASE(session_seed__attach_handshake__31800__address_requested)
{
    seed_peer peer{ [](settings& set)
    {
        set.protocol_maximum = level::headers_protocol;
    } };

    const auto node = peer.handshake(level::headers_protocol);
    BOOST_REQUIRE_EQUAL(node->value, level::headers_protocol);
    BOOST_REQUIRE(get_address::deserialize(level::headers_protocol, peer.receive(get_address::command)));
    peer.close();
    BOOST_REQUIRE_EQUAL(peer.started(), error::seeding_unsuccessful);
}

BOOST_AUTO_TEST_SUITE_END()
