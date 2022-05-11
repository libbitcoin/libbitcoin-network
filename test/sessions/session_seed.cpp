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

BOOST_AUTO_TEST_SUITE(session_seed_tests)

using namespace bc::network;
using namespace bc::network::messages;
using namespace bc::system::chain;

class mock_channel
  : public channel
{
public:
    typedef std::shared_ptr<mock_channel> ptr;

    mock_channel(bool& set, std::promise<bool>& coded,
        const code& match, socket::ptr socket, const settings& settings)
      : channel(socket, settings), match_(match), set_(set), coded_(coded)
    {
    }

    void stop(const code& ec) noexcept override
    {
        // Set future on first code match.
        if (ec == match_ && !set_)
        {
            set_ = true;
            coded_.set_value(true);
        }

        channel::stop(ec);
    }

private:
    const code match_;
    bool& set_;
    std::promise<bool>& coded_;
};

template <error::error_t ChannelStopCode = error::success>
class mock_connector_connect_success
  : public connector
{
public:
    typedef std::shared_ptr<mock_connector_connect_success> ptr;

    using connector::connector;

    // Require template parameterized channel stop code (ChannelStopCode).
    bool require_code() const
    {
        return coded_.get_future().get();
    }

    // Get captured connected.
    virtual bool connected() const
    {
        return !is_zero(connects_);
    }

    // Get captured connection count.
    virtual size_t connects() const
    {
        return connects_;
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

    // Handle connect, capture first connected hostname and port.
    void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) override
    {
        if (is_zero(connects_++))
        {
            hostname_ = hostname;
            port_ = port;
        }

        const auto socket = std::make_shared<network::socket>(service_);
        const auto channel = std::make_shared<mock_channel>(set_, coded_,
            ChannelStopCode, socket, settings_);

        // Must be asynchronous or is an infinite recursion.
        boost::asio::post(strand_, [=]()
        {
            // Connect result code is independent of the channel stop code.
            // As error code would set the re-listener timer, channel pointer is ignored.
            handler(error::success, channel);
        });
    }

protected:
    bool stopped_{ false };
    size_t connects_{ zero };
    std::string hostname_;
    uint16_t port_;
    bool set_{ false };
    mutable std::promise<bool> coded_;
};

class mock_connector_connect_fail
  : public mock_connector_connect_success<error::success>
{
public:
    typedef std::shared_ptr<mock_connector_connect_fail> ptr;

    using mock_connector_connect_success<error::success>::mock_connector_connect_success;

    void connect(const std::string&, uint16_t,
        connect_handler&& handler) override
    {
        boost::asio::post(strand_, [=]()
        {
            handler(error::invalid_magic, nullptr);
        });
    }
};

class mock_session_seed
  : public session_seed
{
public:
    using session_seed::session_seed;

    bool inbound() const noexcept override
    {
        return session_seed::inbound();
    }

    bool notify() const noexcept override
    {
        return session_seed::notify();
    }

    bool stopped() const
    {
        return session_seed::stopped();
    }

    // Capture first start_connect call.
    void start_seed(const config::endpoint& seed,
        connector::ptr connector, channel_handler handler) noexcept override
    {
        // Must be first to ensure connector::connect() preceeds promise release.
        session_seed::start_seed(seed, connector, handler);

        if (!seeded_)
        {
            seeded_ = true;
            seed_.set_value(true);
        }
    }

    bool seeded() const
    {
        return seeded_;
    }

    bool require_seeded() const
    {
        return seed_.get_future().get();
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

private:
    mutable bool seeded_{ false };
    mutable bool handshaked_{ false };
    mutable std::promise<bool> seed_;
    mutable std::promise<bool> handshake_;
};

class mock_session_seed_one_address_count
  : public mock_session_seed
{
public:
    using mock_session_seed::mock_session_seed;

    size_t address_count() const noexcept override
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
    void start(result_handler handler) noexcept
    {
        count_ = zero;
        mock_session_seed::start(handler);
    }

    size_t address_count() const noexcept override
    {
        return count_++;
    }

private:
    mutable size_t count_{ zero };
};

template <class Connector = network::connector>
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

class mock_connector_stop_connect
  : public mock_connector_connect_success<error::service_stopped>
{
public:
    typedef std::shared_ptr<mock_connector_stop_connect> ptr;

    mock_connector_stop_connect(asio::strand& strand, asio::io_context& service,
        const settings& settings, mock_session_seed::ptr session)
      : mock_connector_connect_success(strand, service, settings),
        session_(session)
    {
    }

    void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) noexcept override
    {
        BC_ASSERT_MSG(session_, "call set_session");

        // This connector.connect is invoked from network stranded method.
        session_->stop();

        mock_connector_connect_success<error::service_stopped>::connect(hostname,
            port, std::move(handler));
    }

private:
    mock_session_seed::ptr session_;
};

// Can't derive from mock_p2p because Connector has more arguments.
class mock_p2p_stop_connect
  : public p2p
{
public:
    using p2p::p2p;

    void set_session(mock_session_seed::ptr session)
    {
        session_ = session;
    }

    // Get first created connector.
    mock_connector_stop_connect::ptr get_connector() const
    {
        return connector_;
    }

    // Create mock connector to inject mock channel.
    connector::ptr create_connector() noexcept override
    {
        if (connector_)
            return connector_;

        return ((connector_ = std::make_shared<mock_connector_stop_connect>(
            strand(), service(), network_settings(), session_)));
    }

private:
    mock_connector_stop_connect::ptr connector_;
    mock_session_seed::ptr session_;
};

// properties

BOOST_AUTO_TEST_CASE(session_seed__inbound__always__false)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session_seed session(net);
    BOOST_REQUIRE(!session.inbound());
}

BOOST_AUTO_TEST_CASE(session_seed__notify__always__false)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session_seed session(net);
    BOOST_REQUIRE(!session.notify());
}

// stop

BOOST_AUTO_TEST_CASE(session_seed__stop__started__stopped)
{
    settings set(selection::mainnet);
    set.host_pool_capacity = 1;
    set.outbound_connections = 0;
    mock_p2p<mock_connector_connect_fail> net(set);
    auto session = std::make_shared<mock_session_seed>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            // This will not fire until seeding complete.
            started.set_value(ec);
        });
    });

    // This indicates unsuccessful seeding (start), not connection(s) status.
    // Because p2p is not started, connections will fail until stop.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        // This is blocked above until seeding completes.
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_seed__stop__stopped__stopped)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session_seed session(net);

    std::promise<bool> promise;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        promise.set_value(true);
    });

    BOOST_REQUIRE(promise.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

//start

BOOST_AUTO_TEST_CASE(session_seed__start__no_host_pool_capacity__stopped)
{
    settings set(selection::mainnet);
    set.outbound_connections = 0;
    set.host_pool_capacity = 0;
    p2p net(set);
    auto session = std::make_shared<mock_session_seed>(net);
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
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_seed__start__no_seeds__stopped)
{
    settings set(selection::mainnet);
    set.outbound_connections = 0;
    set.host_pool_capacity = 1;
    set.seeds.clear();
    p2p net(set);
    auto session = std::make_shared<mock_session_seed>(net);
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
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_seed__start__one_address_count__stopped)
{
    settings set(selection::mainnet);
    set.outbound_connections = 0;
    set.host_pool_capacity = 1;
    p2p net(set);
    auto session = std::make_shared<mock_session_seed_one_address_count>(net);
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
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_seed__start__restart__operation_failed)
{
    settings set(selection::mainnet);
    set.outbound_connections = 0;
    set.host_pool_capacity = 1;
    set.seeds.resize(3);
    mock_p2p<mock_connector_connect_fail> net(set);
    auto session = std::make_shared<mock_session_seed_increasing_address_count>(net);
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

    std::promise<code> restarted;
    boost::asio::post(net.strand(), [=, &restarted]()
    {
        session->start([&](const code& ec)
        {
            restarted.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(restarted.get_future().get(), error::operation_failed);
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

BOOST_AUTO_TEST_CASE(session_seed__start__seeded__success)
{
    settings set(selection::mainnet);
    set.outbound_connections = 0;
    set.host_pool_capacity = 1;
    set.seeds.resize(2);
    mock_p2p<mock_connector_connect_success<error::success>> net(set);
    auto session = std::make_shared<mock_session_seed_increasing_address_count>(net);
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

    // No need to block since seeding completes at started true.
    BOOST_REQUIRE(net.get_connector()->connected());
    BOOST_REQUIRE(session->attached_handshake());

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

BOOST_AUTO_TEST_CASE(session_seed__start__stop_during_seeding__success)
{
    settings set(selection::mainnet);
    set.outbound_connections = 0;
    set.host_pool_capacity = 1;
    mock_p2p_stop_connect net(set);

    // Stop is invoked just before first connector.connect, one seed processed.
    auto session = std::make_shared<mock_session_seed_increasing_address_count>(net);
    net.set_session(session);
    BOOST_REQUIRE(session->stopped());
    
    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    // mock_p2p_stop_connect will set stop before started completes.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::service_stopped);
    BOOST_REQUIRE_EQUAL(net.get_connector()->connects(), 1u);
    BOOST_REQUIRE(!session->attached_handshake());
    BOOST_REQUIRE(session->stopped());

    // Must block until start completes before clearing the session.
    net.close();
    session.reset();
}

// start via network (not required for coverage)

BOOST_AUTO_TEST_SUITE_END()
