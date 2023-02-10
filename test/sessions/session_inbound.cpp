/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
class mock_acceptor_start_success_accept_success
  : public acceptor
{
public:
    typedef std::shared_ptr<mock_acceptor_start_success_accept_success> ptr;

    using acceptor::acceptor;

    // Require template parameterized channel stop code (ChannelStopCode).
    bool require_code() const NOEXCEPT
    {
        return coded_.get_future().get();
    }

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

    // Capture stopped and free channel.
    void stop() NOEXCEPT override
    {
        stopped_ = true;
        acceptor::stop();
    }

    // Handle accept.
    void accept(channel_handler&& handler) NOEXCEPT override
    {
        ++accepts_;
        const auto socket = std::make_shared<network::socket>(log(), service_);
        const auto channel = std::make_shared<mock_channel>(log(), set_,
            coded_, ChannelStopCode, socket, settings_);

        // Must be asynchronous or is an infinite recursion.
        // This error code will set the re-listener timer and channel pointer is ignored.
        boost::asio::post(strand_, [=]() NOEXCEPT
        {
            handler(error::success, channel);
        });
    }

protected:
    bool stopped_{ false };
    size_t accepts_{ zero };
    uint16_t port_{ 0 };
    bool set_{ false };
    mutable std::promise<bool> coded_;
};

class mock_acceptor_start_success_accept_fail
  : public mock_acceptor_start_success_accept_success<error::success>
{
public:
    typedef std::shared_ptr<mock_acceptor_start_success_accept_fail> ptr;

    using mock_acceptor_start_success_accept_success<error::success>::
        mock_acceptor_start_success_accept_success;

    // Handle accept with unknown error.
    void accept(channel_handler&& handler) NOEXCEPT override
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
    void accept(channel_handler&& handler) NOEXCEPT override
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
};

class mock_session_inbound
  : public session_inbound
{
public:
    using session_inbound::session_inbound;

    bool inbound() const NOEXCEPT override
    {
        return session_inbound::inbound();
    }

    bool notify() const NOEXCEPT override
    {
        return session_inbound::notify();
    }

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
        result_handler&& handshake) const NOEXCEPT override
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

class mock_session_start_accept_parameter_error
  : public mock_session_inbound
{
public:
    using mock_session_inbound::mock_session_inbound;

    void start_accept(const code&,
        const acceptor::ptr& acceptor) NOEXCEPT override
    {
        mock_session_inbound::start_accept(error::invalid_checksum, acceptor);
    }
};

class mock_session_inbound_blacklist_fail
  : public mock_session_inbound
{
public:
    using mock_session_inbound::mock_session_inbound;

    bool blacklisted(const config::authority&) const NOEXCEPT override
    {
        return true;
    }
};

class mock_session_inbound_whitelist_fail
  : public mock_session_inbound
{
public:
    using mock_session_inbound::mock_session_inbound;

    bool whitelisted(const config::authority&) const NOEXCEPT override
    {
        return false;
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
        return ((acceptor_ = std::make_shared<Acceptor>(log(), strand(),
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

private:
    typename Acceptor::ptr acceptor_;

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

BOOST_AUTO_TEST_CASE(session_inbound__inbound__always__true)
{
    const logger log{};
    settings set(selection::mainnet);
    p2p net(set, log);
    mock_session_inbound session(net);
    BOOST_REQUIRE(session.inbound());
}

BOOST_AUTO_TEST_CASE(session_inbound__notify__always__true)
{
    const logger log{};
    settings set(selection::mainnet);
    p2p net(set, log);
    mock_session_inbound session(net);
    BOOST_REQUIRE(session.notify());
}

// stop

BOOST_AUTO_TEST_CASE(session_inbound__stop__started__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        // Will cause started to be set and acceptor created.
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
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__stopped__stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p<> net(set, log);
    mock_session_inbound session(net);

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

BOOST_AUTO_TEST_CASE(session_inbound__start__no_inbound_connections__bypassed)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 0;
    mock_p2p<> net(set, log);
    mock_session_inbound session(net);
    BOOST_REQUIRE(session.stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        session.start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::bypassed);
    BOOST_REQUIRE(session.stopped());
}

BOOST_AUTO_TEST_CASE(session_inbound__start__port_zero__bypassed)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 0;
    mock_p2p<> net(set, log);
    mock_session_inbound session(net);
    BOOST_REQUIRE(session.stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        session.start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::bypassed);
    BOOST_REQUIRE(session.stopped());
}

BOOST_AUTO_TEST_CASE(session_inbound__start__inbound_connections_restart__operation_failed)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    mock_p2p<> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net);
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
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__start__acceptor_start_failure__not_accepted)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_fail> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    // mock_acceptor_start_fail.start returns invalid_magic, so start_accept aborts.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::invalid_magic);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.inbound_port);
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!net.get_acceptor()->accepted());
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
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
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__start__acceptor_started_accept_returns_stopped__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_stopped> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.inbound_port);
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    // Block until accepted.
    BOOST_REQUIRE(session->require_accepted());
    BOOST_REQUIRE(net.get_acceptor()->accepted());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
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
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__start__acceptor_started__timer_failure_code__no_accept)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_success_accept_success<>> net(set, log);

    // start_accept is invoked with invalid_checksum.
    auto session = std::make_shared<mock_session_start_accept_parameter_error>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.inbound_port);
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!net.get_acceptor()->accepted());
    BOOST_REQUIRE(!session->stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::invalid_checksum);
    BOOST_REQUIRE(net.get_acceptor()->stopped());

    // Attach is not invoked.
    BOOST_REQUIRE(!session->attached_handshake());
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_error__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_success_accept_fail> net(set, log);
    auto session = std::make_shared<mock_session_inbound>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        // Will cause started to be set and acceptor created.
        session->start([=, &started](const code& ec)
        {
            started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.inbound_port);
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    // Block until accepted.
    BOOST_REQUIRE(session->require_accepted());
    BOOST_REQUIRE(net.get_acceptor()->accepted());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
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
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_oversubscribed__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_success_accept_success<error::oversubscribed>> net(set, log);

    std::promise<code> net_started;
    net.start([&](const code& ec)
    {
        net_started.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(net_started.get_future().get(), error::success);

    // Start the session (with one mocked inbound channel) using the network reference.
    auto session = std::make_shared<mock_session_inbound_channel_count_fail>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> session_started;
    boost::asio::post(net.strand(), [=, &session_started]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            session_started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(session_started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.inbound_port);
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    // Block until handle_accept sets oversubscribed in channel.stop.
    BOOST_REQUIRE(net.get_acceptor()->require_code());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
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
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_blacklisted__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_success_accept_success<error::address_blocked>> net(set, log);

    std::promise<code> net_started;
    net.start([&](const code& ec)
    {
        net_started.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(net_started.get_future().get(), error::success);

    // Start the session (with one mocked inbound channel) using the network reference.
    auto session = std::make_shared<mock_session_inbound_blacklist_fail>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> session_started;
    boost::asio::post(net.strand(), [=, &session_started]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            session_started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(session_started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.inbound_port);
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    // Block until handle_accept sets address_blocked in channel.stop.
    BOOST_REQUIRE(net.get_acceptor()->require_code());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
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
    session.reset();
}


BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_not_whitelisted__not_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    mock_p2p<mock_acceptor_start_success_accept_success<error::address_blocked>> net(set, log);

    std::promise<code> net_started;
    net.start([&](const code& ec)
    {
        net_started.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(net_started.get_future().get(), error::success);

    // Start the session (with one mocked inbound channel) using the network reference.
    auto session = std::make_shared<mock_session_inbound_whitelist_fail>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> session_started;
    boost::asio::post(net.strand(), [=, &session_started]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            session_started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(session_started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.inbound_port);
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    // Block until handle_accept sets address_blocked in channel.stop.
    BOOST_REQUIRE(net.get_acceptor()->require_code());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
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
    session.reset();
}

BOOST_AUTO_TEST_CASE(session_inbound__stop__acceptor_started_accept_success__attached)
{
    const logger log{};
    settings set(selection::mainnet);
    set.inbound_connections = 1;
    set.inbound_port = 42;
    set.connect_timeout_seconds = 10000;
    mock_p2p<mock_acceptor_start_success_accept_success<error::service_stopped>> net(set, log);

    std::promise<code> net_started;
    net.start([&](const code& ec)
    {
        net_started.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(net_started.get_future().get(), error::success);

    // Start the session using the network reference.
    auto session = std::make_shared<mock_session_inbound_handshake_failure>(net);
    BOOST_REQUIRE(session->stopped());

    std::promise<code> session_started;
    boost::asio::post(net.strand(), [=, &session_started]()
    {
        // Will cause started to be set and acceptor created.
        session->start([&](const code& ec)
        {
            session_started.set_value(ec);
        });
    });

    // mock_acceptor_start_success.start returns success, so start_accept invokes accept.accept.
    BOOST_REQUIRE_EQUAL(session_started.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(net.get_acceptor()->port(), set.inbound_port);
    BOOST_REQUIRE(!net.get_acceptor()->stopped());
    BOOST_REQUIRE(!session->stopped());

    // Block until accepted.
    BOOST_REQUIRE(session->require_accepted());
    BOOST_REQUIRE(session->require_attached_handshake());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    // Block until handle_accept sets service_stopped in channel.stop.
    BOOST_REQUIRE(net.get_acceptor()->require_code());

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE_EQUAL(session->start_accept_code(), error::success);
    BOOST_REQUIRE(net.get_acceptor()->stopped());

    // Handshake protocols attached.
    BOOST_REQUIRE(session->attached_handshake());
    session.reset();
}

BOOST_AUTO_TEST_SUITE_END()
