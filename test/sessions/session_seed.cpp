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

using namespace bc::network::messages;
using namespace bc::system::chain;

class mock_channel
  : public channel
{
public:
    typedef std::shared_ptr<mock_channel> ptr;

    mock_channel(const logger& log, bool& set, std::promise<bool>& coded,
        const code& match, socket::ptr socket, const settings& settings) NOEXCEPT
      : channel(log, socket, settings), match_(match), set_(set), coded_(coded)
    {
    }

    void stop(const code& ec) NOEXCEPT override
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
    bool require_code() const NOEXCEPT
    {
        return coded_.get_future().get();
    }

    // Get captured connected.
    bool connected() const NOEXCEPT
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
    void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) NOEXCEPT override
    {
        if (is_zero(connects_++))
        {
            hostname_ = hostname;
            port_ = port;
        }

        const auto socket = std::make_shared<network::socket>(log(), service_);
        const auto channel = std::make_shared<mock_channel>(log(), set_, coded_,
            ChannelStopCode, socket, settings_);

        // Must be asynchronous or is an infinite recursion.
        boost::asio::post(strand_, [=]() NOEXCEPT
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
        connect_handler&& handler) NOEXCEPT override
    {
        boost::asio::post(strand_, [=]() NOEXCEPT
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

    bool inbound() const NOEXCEPT override
    {
        return session_seed::inbound();
    }

    bool notify() const NOEXCEPT override
    {
        return session_seed::notify();
    }

    bool stopped() const NOEXCEPT override
    {
        return session_seed::stopped();
    }

    // Capture first start_connect call.
    void start_seed(const config::endpoint& seed,
        const connector::ptr& connector,
        const channel_handler& handler) NOEXCEPT override
    {
        // Must be first to ensure connector::connect() preceeds promise release.
        session_seed::start_seed(seed, connector, handler);

        if (!seeded_)
        {
            seeded_ = true;
            seed_.set_value(true);
        }
    }

    bool seeded() const NOEXCEPT
    {
        return seeded_;
    }

    bool require_seeded() const NOEXCEPT
    {
        return seed_.get_future().get();
    }

    void attach_handshake(const channel::ptr&,
        result_handler&& handshake) const NOEXCEPT override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
            handshake_.set_value(true);
        }

        // Simulate handshake successful completion.
        handshake(error::success);
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

template <class Connector = connector>
class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    // Get last created connector.
    typename Connector::ptr get_connector() const NOEXCEPT
    {
        return connector_;
    }

    // Create mock connector to inject mock channel.
    connector::ptr create_connector() NOEXCEPT override
    {
        return ((connector_ = std::make_shared<Connector>(log(), strand(),
            service(), network_settings())));
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

    code store(const channel::ptr&, bool, bool) NOEXCEPT override
    {
        return error::success;
    }

    code unstore(const channel::ptr&, bool) NOEXCEPT override
    {
        return error::success;
    }

    void saves(const messages::address_items& addresses,
        result_handler&& complete) NOEXCEPT override
    {
        hosts_ += addresses.size();
        complete(error::success);
    }

    size_t address_count() const NOEXCEPT override
    {
        return hosts_;
    }

private:
    typename Connector::ptr connector_;

    class mock_inbound_session
      : public session_inbound
    {
    public:
        mock_inbound_session(p2p& network) NOEXCEPT
          : session_inbound(network)
        {
        }

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };

    class mock_outbound_session
      : public session_outbound
    {
    public:
        mock_outbound_session(p2p& network) NOEXCEPT
          : session_outbound(network)
        {
        }

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };

    class mock_seed_session
      : public session_seed
    {
    public:
        mock_seed_session(p2p& network) NOEXCEPT
          : session_seed(network)
        {
        }

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };

    size_t hosts_{};
};

class mock_connector_stop_connect
  : public mock_connector_connect_success<error::service_stopped>
{
public:
    typedef std::shared_ptr<mock_connector_stop_connect> ptr;

    mock_connector_stop_connect(const logger& log, asio::strand& strand,
        asio::io_context& service, const settings& settings,
        mock_session_seed::ptr session) NOEXCEPT
      : mock_connector_connect_success(log, strand, service, settings),
        session_(session)
    {
    }

    void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler) NOEXCEPT override
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
    connector::ptr create_connector() NOEXCEPT override
    {
        if (connector_)
            return connector_;

        return ((connector_ = std::make_shared<mock_connector_stop_connect>(
            log(), strand(), service(), network_settings(), session_)));
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
        mock_inbound_session(p2p& network) NOEXCEPT
          : session_inbound(network)
        {
        }

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };

    class mock_outbound_session
      : public session_outbound
    {
    public:
        mock_outbound_session(p2p& network) NOEXCEPT
          : session_outbound(network)
        {
        }

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };

    class mock_seed_session
      : public session_seed
    {
    public:
        mock_seed_session(p2p& network) NOEXCEPT
          : session_seed(network)
        {
        }

        void start(result_handler&& handler) NOEXCEPT override
        {
            handler(error::success);
        }
    };
};

// properties

BOOST_AUTO_TEST_CASE(session_seed__inbound__always__false)
{
    const logger log{};
    settings set(selection::mainnet);
    p2p net(set, log);
    mock_session_seed session(net);
    BOOST_REQUIRE(!session.inbound());
}

BOOST_AUTO_TEST_CASE(session_seed__notify__always__false)
{
    const logger log{};
    settings set(selection::mainnet);
    p2p net(set, log);
    mock_session_seed session(net);
    BOOST_REQUIRE(!session.notify());
}

// stop

BOOST_AUTO_TEST_CASE(session_seed__stop__started__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound_connections = 1;
    set.host_pool_capacity = 1;
    mock_p2p_stop_connect net(set, log);

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

    // Block until started connectors/channels complete before clearing session.
    net.close();
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_seed__stop__stopped__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<> net(set, log);
    mock_session_seed session(net);
    BOOST_REQUIRE(session.stopped());

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

BOOST_AUTO_TEST_CASE(session_seed__start__no_outbound__bypassed)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound_connections = 0;
    mock_p2p<> net(set, log);
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

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::bypassed);
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_seed__start__outbound_one_address_count__bypassed)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound_connections = 1;
    set.host_pool_capacity = 1;
    mock_p2p<> net(set, log);
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

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::bypassed);
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_seed__start__outbound_no_host_pool_capacity__seeding_unsuccessful)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound_connections = 1;
    set.host_pool_capacity = 0;
    mock_p2p<> net(set, log);
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

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_seed__start__outbound_no_seeds__seeding_unsuccessful)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound_connections = 1;
    set.host_pool_capacity = 1;
    set.seeds.clear();
    mock_p2p<> net(set, log);
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

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_seed__start__restart__operation_failed)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound_connections = 1;
    set.host_pool_capacity = 1;
    set.seeds.resize(3);
    mock_p2p<mock_connector_connect_fail> net(set, log);
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
    const logger log{};
    settings set(selection::mainnet);
    set.outbound_connections = 1;
    set.host_pool_capacity = 1;
    set.seeds.resize(2);
    mock_p2p<mock_connector_connect_success<error::success>> net(set, log);
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

BOOST_AUTO_TEST_CASE(session_seed__start__not_seeded__seeding_unsuccessful)
{
    const logger log{};
    settings set(selection::mainnet);
    set.outbound_connections = 1;
    set.host_pool_capacity = 1;
    mock_p2p<mock_connector_connect_success<error::success>> net(set, log);
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

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::seeding_unsuccessful);
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(net.get_connector()->connected());
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    session.reset();
}

////BOOST_AUTO_TEST_CASE(session_seed__live__one_address__expected)
////{
////    const logger log{};
////    settings set(selection::mainnet);
////    set.seeds.resize(1);
////    set.channel_germination_seconds = 5;
////    set.outbound_connections = 1;
////    set.host_pool_capacity = 1;
////    mock_p2p<> net(set, log);
////    auto session = std::make_shared<session_seed>(net);
////
////    std::promise<code> started;
////    boost::asio::post(net.strand(), [=, &started]()
////    {
////        session->start([&](const code& ec)
////        {
////            started.set_value(ec);
////        });
////    });
////
////    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
////
////    std::promise<bool> stopped;
////    boost::asio::post(net.strand(), [=, &stopped]()
////    {
////        session->stop();
////        stopped.set_value(true);
////    });
////
////    BOOST_REQUIRE(stopped.get_future().get());
////    BOOST_REQUIRE_GT(net.address_count(), zero);
////    session.reset();
////}

BOOST_AUTO_TEST_SUITE_END()
