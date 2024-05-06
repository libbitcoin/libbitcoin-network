/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(session_inbound_tests)

using namespace bc::system::chain;

class mock_acceptor_start_success_accept_success
  : public acceptor
{
public:
    typedef std::shared_ptr<mock_acceptor_start_success_accept_success> ptr;

    using acceptor::acceptor;

    // Get captured port.
    uint16_t port() const NOEXCEPT
    {
        return port_;
    }

    // Get captured accepted.
    bool accepted() const NOEXCEPT
    {
        return !is_zero(accepts_);
    }

    // Get captured stopped.
    bool stopped() const NOEXCEPT
    {
        return stopped_;
    }

    // Capture port, succeed on first (others prevents tight success loop).
    code start(uint16_t port) NOEXCEPT override
    {
        port_ = port;
        return !accepted() ? error::success : error::unknown;
    }

    // Capture port, succeed on first (others prevents tight success loop).
    code start(const config::authority& local) NOEXCEPT override
    {
        port_ = local.port();
        return !accepted() ? error::success : error::unknown;
    }

    // Capture stopped and free channel.
    void stop() NOEXCEPT override
    {
        stopped_ = true;
        acceptor::stop();
    }

    // Handle accept.
    void accept(socket_handler&& handler) NOEXCEPT override
    {
        ++accepts_;
        const auto socket = std::make_shared<network::socket>(log, service_);

        // Must be asynchronous or is an infinite recursion.
        // This error code will set the re-listener timer and channel pointer is ignored.
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            handler(error::success, socket);
        });
    }

protected:
    bool stopped_{ false };
    size_t accepts_{ zero };
    uint16_t port_{ 0 };
};

class mock_acceptor_start_success_accept_fail
  : public mock_acceptor_start_success_accept_success
{
public:
    typedef std::shared_ptr<mock_acceptor_start_success_accept_fail> ptr;

    using mock_acceptor_start_success_accept_success::
        mock_acceptor_start_success_accept_success;

    // Handle accept with unknown error.
    void accept(socket_handler&& handler) NOEXCEPT override
    {
        ++accepts_;
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            handler(error::unknown, nullptr);
        });
    }
};

class mock_acceptor_start_stopped
  : public mock_acceptor_start_success_accept_fail
{
public:
    typedef std::shared_ptr<mock_acceptor_start_stopped> ptr;

    using mock_acceptor_start_success_accept_fail::
        mock_acceptor_start_success_accept_fail;

    // Handle accept with service_stopped error.
    void accept(socket_handler&& handler) NOEXCEPT override
    {
        ++accepts_;
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            handler(error::service_stopped, nullptr);
        });
    }
};

class mock_acceptor_start_fail
  : public mock_acceptor_start_success_accept_fail
{
public:
    typedef std::shared_ptr<mock_acceptor_start_fail> ptr;

    using mock_acceptor_start_success_accept_fail::
        mock_acceptor_start_success_accept_fail;

    // Capture port, fail.
    code start(uint16_t port) NOEXCEPT override
    {
        port_ = port;
        return error::invalid_magic;
    }

    // Capture port, fail.
    code start(const config::authority& local) NOEXCEPT override
    {
        port_ = local.port();
        return error::invalid_magic;
    }
};

class mock_session_inbound
  : public session_inbound
{
public:
    using session_inbound::session_inbound;

    bool stopped() const NOEXCEPT override
    {
        return session_inbound::stopped();
    }

    code start_accept_code() const NOEXCEPT
    {
        return start_accept_code_;
    }

    // Capture first start_accept call.
    void start_accept(const code& ec,
        const acceptor::ptr& acceptor) NOEXCEPT override
    {
        // Must be first to ensure acceptor::accept() preceeds promise release.
        session_inbound::start_accept(ec, acceptor);

        if (!accepted_)
        {
            accepted_ = true;
            start_accept_code_ = ec;
            accept_.set_value(true);
        }
    }

    bool accepted() const NOEXCEPT
    {
        return accepted_;
    }

    bool require_accepted() const NOEXCEPT
    {
        return accept_.get_future().get();
    }

    void attach_handshake(const channel::ptr&,
        result_handler&& handshake) NOEXCEPT override
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

    bool require_attached_handshake() const
    {
        return handshake_.get_future().get();
    }

protected:
    mutable bool handshaked_{ false };
    mutable std::promise<bool> handshake_;

private:
    code start_accept_code_{ error::unknown };
    mutable bool accepted_{ false };
    mutable std::promise<bool> accept_;
};

class mock_session_inbound_handshake_failure
  : public mock_session_inbound
{
public:
    using mock_session_inbound::mock_session_inbound;

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

class mock_session_inbound_channel_count_fail
  : public mock_session_inbound
{
public:
    using mock_session_inbound::mock_session_inbound;

    size_t inbound_channel_count() const NOEXCEPT override
    {
        return 1;
    }
};

////class mock_session_start_accept_parameter_error
////  : public mock_session_inbound
////{
////public:
////    using mock_session_inbound::mock_session_inbound;
////
////    void start_accept(const code&,
////        const acceptor::ptr& acceptor) NOEXCEPT override
////    {
////        mock_session_inbound::start_accept(error::invalid_checksum, acceptor);
////    }
////};

class mock_session_inbound_whitelist_fail
  : public mock_session_inbound
{
public:
    using mock_session_inbound::mock_session_inbound;

    bool whitelisted(const config::address&) const NOEXCEPT override
    {
        return false;
    }
};

class mock_session_inbound_blacklist_fail
  : public mock_session_inbound
{
public:
    using mock_session_inbound::mock_session_inbound;

    bool blacklisted(const config::address&) const NOEXCEPT override
    {
        return true;
    }
};

template <class Acceptor = acceptor>
class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    typename Acceptor::ptr get_acceptor() const NOEXCEPT
    {
        return acceptor_;
    }

    // Create mock acceptor to inject mock channel.
    acceptor::ptr create_acceptor() NOEXCEPT override
    {
        return ((acceptor_ = std::make_shared<Acceptor>(log, strand(),
            service(), network_settings(), suspended_)));
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
    typename Acceptor::ptr acceptor_;
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

BOOST_AUTO_TEST_CASE(session_inbound__stop__started__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        // Will cause started to be set and acceptor created.
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
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__stopped__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<> net(set, log);
    mock_session_inbound session(net, 1);

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

BOOST_AUTO_TEST_CASE(session_inbound__start__no_inbound_connections__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 0;
    mock_p2p<> net(set, log);
    mock_session_inbound session(net, 1);
    BOOST_REQUIRE(session.stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session.start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(session.stopped());
}

BOOST_AUTO_TEST_CASE(session_inbound__start__empty_binds__success)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.binds.clear();
    mock_p2p<> net(set, log);
    mock_session_inbound session(net, 1);
    BOOST_REQUIRE(session.stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session.start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(session.stopped());
}

BOOST_AUTO_TEST_CASE(session_inbound__start__inbound_connections_restart__operation_failed)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net, 1);
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
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);
}

BOOST_AUTO_TEST_CASE(session_inbound__start__acceptor_start_failure__not_accepted)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<mock_acceptor_start_fail> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net, 1);
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(set.binds.size(), one);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    // mock_acceptor_start_fail.start returns invalid_magic, so start_accept aborts.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::invalid_magic);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.binds.front().port());
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!net.get_acceptor()->accepted());
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());

    // start_accept is not invoked in the case of a start error.
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::unknown);

    // acceptor.stop is not called because acceptor.start failed.
    BOOST_REQUIRE(!net.get_acceptor()->stopped());

    // Attach is not invoked.
    BOOST_REQUIRE(!session->attached_handshake());
}

BOOST_AUTO_TEST_CASE(session_inbound__start__acceptor_started_accept_returns_stopped__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<mock_acceptor_start_stopped> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net, 1);
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(set.binds.size(), one);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.binds.front().port());
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    // Block until accepted.
    BOOST_REQUIRE(session->require_accepted());
    BOOST_REQUIRE(net.get_acceptor()->accepted());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);
    BOOST_REQUIRE(net.get_acceptor()->stopped());

    // Not attached because accept returned stopped.
    BOOST_REQUIRE(!session->attached_handshake());
}

// start_accept(error) is not currently handled in any session type (no scenario).
////BOOST_AUTO_TEST_CASE(session_inbound__start__acceptor_started__timer_failure_code__no_accept)
////{
////    const logger log{};
////    settings set(selection::mainnet);
////    set.inbound_connections = 1;
////    mock_p2p<mock_acceptor_start_success_accept_success> net(set, log);
////
////    // start_accept is invoked with invalid_checksum.
////    auto session = std::make_shared<mock_session_start_accept_parameter_error>(net, 1);
////    BOOST_REQUIRE(session->stopped());
////    BOOST_REQUIRE_EQUAL(set.binds.size(), one);
////
////    std::promise<code> started;
////    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
////    {
////        // Will cause started to be set and acceptor created.
////        session->start([&](const code& ec) NOEXCEPT
////        {
////            started.set_value(ec);
////        });
////    });
////
////    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
////    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.binds.front().port());
////    BOOST_REQUIRE(!net.get_acceptor()->stopped());
////    BOOST_REQUIRE(!net.get_acceptor()->accepted());
////    BOOST_REQUIRE(!session->stopped());
////
////    std::promise<bool> stopped;
////    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
////    {
////        session->stop();
////        stopped.set_value(true);
////    });
////
////    BOOST_REQUIRE(stopped.get_future().get());
////    BOOST_REQUIRE(session->stopped());
////    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::invalid_checksum);
////    BOOST_REQUIRE(net.get_acceptor()->stopped());
////
////    // Attach is not invoked.
////    BOOST_REQUIRE(!session->attached_handshake());
////}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_error__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<mock_acceptor_start_success_accept_fail> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net, 1);
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(set.binds.size(), one);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        // Will cause started to be set and acceptor created.
        session->start([=, &started](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.binds.front().port());
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    // Block until accepted.
    BOOST_REQUIRE(session->require_accepted());
    BOOST_REQUIRE(net.get_acceptor()->accepted());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);
    BOOST_REQUIRE(net.get_acceptor()->stopped());

    // Not attached because accept returned error.
    BOOST_REQUIRE(!session->attached_handshake());
}

// Socket termination (sockets have no stop codes).

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_not_whitelisted__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<mock_acceptor_start_success_accept_success> net(set, log);
    BOOST_REQUIRE_EQUAL(set.binds.size(), one);

    std::promise<code> net_started;
    net.start([&](const code& ec) NOEXCEPT
    {
        net_started.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(net_started.get_future().get(), error::success);

    // Start the session (with one mocked inbound channel) using the network reference.
    auto session = std::make_shared<mock_session_inbound_whitelist_fail>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> session_started;
    boost::asio::post(net.strand(), [=, &session_started]() NOEXCEPT
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec) NOEXCEPT
        {
            session_started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(session_started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.binds.front().port());
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(net.get_acceptor()->stopped());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);

    // Not attached because accept never succeeded.
    BOOST_REQUIRE(!session->attached_handshake());
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_blacklisted__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<mock_acceptor_start_success_accept_success> net(set, log);
    BOOST_REQUIRE_EQUAL(set.binds.size(), one);

    std::promise<code> net_started;
    net.start([&](const code& ec) NOEXCEPT
    {
        net_started.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(net_started.get_future().get(), error::success);

    // Start the session (with one mocked inbound channel) using the network reference.
    auto session = std::make_shared<mock_session_inbound_blacklist_fail>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> session_started;
    boost::asio::post(net.strand(), [=, &session_started]() NOEXCEPT
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec) NOEXCEPT
        {
            session_started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(session_started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.binds.front().port());
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(net.get_acceptor()->stopped());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);

    // Not attached because accept never succeeded.
    BOOST_REQUIRE(!session->attached_handshake());
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_oversubscribed__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<mock_acceptor_start_success_accept_success> net(set, log);
    BOOST_REQUIRE_EQUAL(set.binds.size(), one);

    std::promise<code> net_started;
    net.start([&](const code& ec)
    {
        net_started.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(net_started.get_future().get(), error::success);

    // Start the session (with one mocked inbound channel) using the network reference.
    auto session = std::make_shared<mock_session_inbound_channel_count_fail>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> session_started;
    boost::asio::post(net.strand(), [=, &session_started]() NOEXCEPT
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec) NOEXCEPT
        {
            session_started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(session_started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.binds.front().port());
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);
    BOOST_REQUIRE(net.get_acceptor()->stopped());

    // Not attached because accept never succeeded.
    BOOST_REQUIRE(!session->attached_handshake());
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_success__attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.connect_timeout_seconds = 10000;
    mock_p2p<mock_acceptor_start_success_accept_success> net(set, log);
    BOOST_REQUIRE_EQUAL(set.binds.size(), one);

    std::promise<code> net_started;
    net.start([&](const code& ec) NOEXCEPT
    {
        net_started.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(net_started.get_future().get(), error::success);

    // Start the session using the network reference.
    auto session = std::make_shared<mock_session_inbound_handshake_failure>(net, 1);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> session_started;
    boost::asio::post(net.strand(), [=, &session_started]() NOEXCEPT
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec) NOEXCEPT
        {
            session_started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(session_started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.binds.front().port());
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    // Block until accepted.
    BOOST_REQUIRE(session->require_accepted());
    BOOST_REQUIRE(session->require_attached_handshake());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);
    BOOST_REQUIRE(net.get_acceptor()->stopped());

    // Handshake protocols attached.
    BOOST_REQUIRE(session->attached_handshake());
}

BOOST_AUTO_TEST_SUITE_END()
