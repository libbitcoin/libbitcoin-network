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

BOOST_AUTO_TEST_SUITE(session_tests)

using namespace bc::network;
using namespace bc::system::chain;
using namespace bc::network::messages;

class mock_channel
    : public channel
{
public:
    mock_channel(socket::ptr socket, const settings& settings)
      : channel(socket, settings)
    {
    }

    void begin() override
    {
        begun_ = true;
        channel::begin();
    }

    bool begun() const
    {
        return begun_;
    }

    void stop(const code& ec) override
    {
        stop_code_ = ec;
        channel::stop(ec);
    }

    void stopper(const code& ec)
    {
        channel::do_stop(ec);
    }

    code stop_code() const
    {
        return stop_code_;
    }

private:
    std::atomic<bool> begun_;
    std::atomic<code> stop_code_;
};

class mock_p2p
  : public p2p
{
public:
    mock_p2p(const settings& settings)
      : p2p(settings)
    {
    }

    uint64_t pent_nonce() const
    {
        return pend_;
    }

    uint64_t unpent_nonce() const
    {
        return unpend_;
    }

    uint64_t stored_nonce() const
    {
        return store_nonce_;
    }

    bool stored_inbound() const
    {
        return store_inbound_;
    }

    bool stored_notify() const
    {
        return store_notify_;
    }

    code stored_result() const
    {
        return store_result_;
    }

    uint64_t unstored_nonce() const
    {
        return unstore_nonce_;
    }

    bool unstored_inbound() const
    {
        return unstore_inbound_;
    }

    bool unstore_found() const
    {
        return unstore_found_;
    }

protected:
    void pend(uint64_t nonce) override
    {
        BC_ASSERT(!is_zero(nonce));
        pend_ = nonce;
        p2p::pend(nonce);
    }

    void unpend(uint64_t nonce) override
    {
        BC_ASSERT(!is_zero(nonce));
        unpend_ = nonce;
        p2p::unpend(nonce);
    }

    code store(channel::ptr channel, bool notify, bool inbound) override
    {
        BC_ASSERT(!is_zero(channel->nonce()));
        store_nonce_ = channel->nonce();
        store_notify_ = notify;
        store_inbound_ = inbound;
        return ((store_result_ = p2p::store(channel, notify, inbound)));
    }

    bool unstore(channel::ptr channel, bool inbound) override
    {
        BC_ASSERT(!is_zero(channel->nonce()));
        unstore_nonce_ = channel->nonce();
        unstore_inbound_ = inbound;
        return ((unstore_found_ = p2p::unstore(channel, inbound)));
    }

private:
    std::atomic<uint64_t> pend_;
    std::atomic<uint64_t> unpend_;

    std::atomic<uint64_t> store_nonce_;
    std::atomic<bool> store_notify_;
    std::atomic<bool> store_inbound_;
    std::atomic<code> store_result_;

    std::atomic<uint64_t> unstore_nonce_;
    std::atomic<bool> unstore_inbound_;
    std::atomic<bool> unstore_found_;
};

class mock_session
  : public session
{
public:
    mock_session(p2p& network)
      : session(network)
    {
    }

    virtual ~mock_session()
    {
    }

    bool stopped() const noexcept
    {
        return session::stopped();
    }

    bool stranded() const
    {
        return session::stranded();
    }

    size_t address_count() const
    {
        return session::address_count();
    }

    size_t channel_count() const
    {
        return session::channel_count();
    }

    size_t inbound_channel_count() const
    {
        return session::inbound_channel_count();
    }

    bool blacklisted(const config::authority& authority) const
    {
        return session::blacklisted(authority);
    }

    bool inbound() const noexcept
    {
        return session::inbound();
    }

    bool notify() const noexcept
    {
        return session::notify();
    }

    void start_channel(channel::ptr channel, result_handler started,
        result_handler stopped) override
    {
        session::start_channel(channel, started, stopped);
    }

    // Handshake

    void attach_handshake(const channel::ptr& channel,
        result_handler handshake) const override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
            handshake_.set_value(true);
        }

        // Simulate handshake completion.
        handshake(channel->stopped() ? error::channel_stopped : error::success);
    }

    bool attached_handshake() const
    {
        return handshaked_;
    }

    bool require_attached_handshake() const
    {
        return handshake_.get_future().get();
    }

    // Protocols

    void attach_protocols(const channel::ptr&) const override
    {
        if (!protocoled_)
        {
            protocoled_ = true;
            protocols_.set_value(true);
        }
    }

    bool attached_protocol() const
    {
        return protocoled_;
    }

    bool require_attached_protocol() const
    {
        return protocols_.get_future().get();
    }

private:
    mutable bool handshaked_{ false };
    mutable bool protocoled_{ false };
    mutable std::promise<bool> handshake_;
    mutable std::promise<bool> protocols_;
};

// construct/settings

BOOST_AUTO_TEST_CASE(session__construct__always__expected_settings)
{
    constexpr auto expected = 42u;
    settings set(selection::mainnet);
    set.threads = expected;
    p2p net(set);
    mock_session session(net);
    BOOST_REQUIRE_EQUAL(session.settings().threads, expected);
}

// properties

BOOST_AUTO_TEST_CASE(session__properties__default__expected)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session session(net);
    BOOST_REQUIRE(session.stopped());
    BOOST_REQUIRE(!session.stranded());
    BOOST_REQUIRE_EQUAL(session.address_count(), zero);
    BOOST_REQUIRE_EQUAL(session.channel_count(), zero);
    BOOST_REQUIRE_EQUAL(session.inbound_channel_count(), zero);;
    BOOST_REQUIRE(!session.blacklisted({ "[2001:db8::2]", 42 }));
    BOOST_REQUIRE(session.notify());
    BOOST_REQUIRE(!session.inbound());
}

// utilities

BOOST_AUTO_TEST_CASE(session__utilities__always__expected)
{
    BOOST_REQUIRE(true);
}

// stop

BOOST_AUTO_TEST_CASE(session__stop__stopped__true)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session session(net);
    BOOST_REQUIRE(session.stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
}

// start

BOOST_AUTO_TEST_CASE(session__start__stop__stopped)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session session(net);

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        session.start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session.stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

// start_channel

BOOST_AUTO_TEST_CASE(session__start_channel__session_not_started__handlers_service_stopped_channel_service_stopped_not_pent_or_stored)
{
    settings set(selection::mainnet);
    mock_p2p net(set);
    auto session = std::make_shared<mock_session>(net);
    BOOST_REQUIRE(session->stopped());

    const auto socket = std::make_shared<network::socket>(net.service());
    const auto channel = std::make_shared<mock_channel>(socket,
        session->settings());

    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    BOOST_REQUIRE(!session->attached_handshake());
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);

    // Channel stopped early due to session being stopped (not started).
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::service_stopped);

    // Channel was not pent or stored.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), 0u);
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), 0u);

    // Channel was not unpent or unstored.
    BOOST_REQUIRE_EQUAL(net.unpent_nonce(), 0u);
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), 0u);
}

BOOST_AUTO_TEST_CASE(session__start_channel__channel_not_started__handlers_channel_stopped_channel_channel_stopped_pent_not_stored)
{
    settings set(selection::mainnet);
    mock_p2p net(set);
    auto session = std::make_shared<mock_session>(net);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.service());
    const auto channel = std::make_shared<mock_channel>(socket,
        session->settings());

    // Stop the channel (started by default).
    std::promise<bool> unstarted_channel;
    boost::asio::post(channel->strand(), [=, &unstarted_channel]()
    {
        channel->stopper(error::invalid_magic);
        unstarted_channel.set_value(true);
    });

    BOOST_REQUIRE(unstarted_channel.get_future().get());
    BOOST_REQUIRE(channel->stopped());

    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::channel_stopped);
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::channel_stopped);
    BOOST_REQUIRE(!session->attached_handshake());

    // Channel was already stopped, but re-stopped explicitly with expected code.
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::channel_stopped);

    // Channel was pent (handshake invoked) and not stored.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), 0u);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());

    // Channel was unpent (handshake completed) but not found on unstore.
    BOOST_REQUIRE_EQUAL(net.unpent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE(!net.unstored_inbound());
    BOOST_REQUIRE(!net.unstore_found());
}

BOOST_AUTO_TEST_CASE(session__start_channel__network_not_started__handlers_service_stopped_channel_service_stopped_pent_store_failed)
{
    settings set(selection::mainnet);
    mock_p2p net(set);
    auto session = std::make_shared<mock_session>(net);

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.service());
    const auto channel = std::make_shared<mock_channel>(socket,
        session->settings());

    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(session->require_attached_handshake());

    // Channel stopped by heading read fail, then by network.store code (network stopped).
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::service_stopped);

    // Channel was pent (handshake invoked) and store failed.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_result(), error::service_stopped);
    BOOST_REQUIRE(!net.stored_inbound());
    BOOST_REQUIRE(net.stored_notify());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());

    // Channel was unpent (handshake completed) but not found on unstore.
    BOOST_REQUIRE_EQUAL(net.unpent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE(!net.unstored_inbound());
    BOOST_REQUIRE(!net.unstore_found());
}

BOOST_AUTO_TEST_CASE(session__start_channel__all_started__handlers_expected_channel_service_stopped_pent_store_succeeded)
{
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    mock_p2p net(set);
    auto session = std::make_shared<mock_session>(net);

    std::promise<code> started_net;
    boost::asio::post(net.strand(), [&net, &started_net]()
    {
        net.start([&](const code& ec)
        {
            started_net.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started_net.get_future().get(), error::success);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]()
    {
        session->start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.service());
    const auto channel = std::make_shared<mock_channel>(socket,
        session->settings());
    
    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]()
    {
        session->start_channel(channel,
            [&](const code& ec)
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec)
            {
                stopped_channel.set_value(ec);
            });
    });

    // Channel stopped by heading read fail.
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::success);
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::channel_stopped);
    BOOST_REQUIRE(session->require_attached_handshake());

    // Channel stopped by heading read fail, stop method called by session.
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::channel_stopped);

    // Channel pent and store succeeded.
    BOOST_REQUIRE_EQUAL(net.pent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.stored_result(), error::success);
    BOOST_REQUIRE(!net.stored_inbound());
    BOOST_REQUIRE(net.stored_notify());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]()
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());

    // Channel was unpent but and found on unstore.
    BOOST_REQUIRE_EQUAL(net.unpent_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE(!net.unstored_inbound());
    BOOST_REQUIRE(net.unstore_found());
}

// attach_handshake
// attach_protocols
// create_acceptor
// create_connector
// create_connectors

BOOST_AUTO_TEST_CASE(session__inbound__always__false)
{
    settings set(selection::mainnet);
    p2p net(set);
    mock_session session(net);
    BOOST_REQUIRE(!session.inbound());
}

BOOST_AUTO_TEST_SUITE_END()
