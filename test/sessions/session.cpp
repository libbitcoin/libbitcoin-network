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

BOOST_AUTO_TEST_SUITE(session_tests)

using namespace bc::network::messages;
using namespace bc::system::chain;

class mock_channel
  : public channel
{
public:
    using channel::channel;

    void resume() NOEXCEPT override
    {
        if (resumed_)
            reresumed_ = true;
        else
            resumed_ = true;

        channel::resume();
    }

    bool resumed() const NOEXCEPT
    {
        return resumed_;
    }

    bool reresumed() const NOEXCEPT
    {
        return reresumed_;
    }

    void stop(const code& ec) NOEXCEPT override
    {
        stop_code_ = ec;
        channel::stop(ec);
    }

    void stopper(const code& ec) NOEXCEPT
    {
        channel::stop(ec);
    }

    code stop_code() const NOEXCEPT
    {
        return stop_code_;
    }

protected:
    bool paused_{ false };
    bool resumed_{ false };
    bool reresumed_{ false };
    code stop_code_{ error::success };
};

class mock_channel_no_read
  : public mock_channel
{
public:
    using mock_channel::mock_channel;

    void resume() NOEXCEPT override
    {
        if (resumed_)
            reresumed_ = true;
        else
            resumed_ = true;

        ////channel::resume();
    }
};

class mock_session
  : public session
{
public:
    mock_session(p2p& network, size_t key) NOEXCEPT
      : session(network, key)
    {
    }

    virtual ~mock_session() NOEXCEPT
    {
    }

    bool stopped() const NOEXCEPT override
    {
        return session::stopped();
    }

    bool stranded() const NOEXCEPT override
    {
        return session::stranded();
    }

    acceptor::ptr create_acceptor() NOEXCEPT override
    {
        return session::create_acceptor();
    }

    connector::ptr create_connector() NOEXCEPT override
    {
        return session::create_connector();
    }

    connectors_ptr create_connectors(size_t count) NOEXCEPT override
    {
        return session::create_connectors(count);
    }

    ////size_t address_count() const NOEXCEPT override
    ////{
    ////    return session::address_count();
    ////}

    size_t channel_count() const NOEXCEPT override
    {
        return session::channel_count();
    }

    size_t inbound_channel_count() const NOEXCEPT override
    {
        return session::inbound_channel_count();
    }

    void start_channel(const channel::ptr& channel, result_handler&& started,
        result_handler&& stopped) NOEXCEPT override
    {
        session::start_channel(channel, std::move(started), std::move(stopped));
    }

    void attach_handshake(const channel::ptr& channel,
        result_handler&& handshake) NOEXCEPT override
    {
        if (!handshaked_)
        {
            handshaked_ = true;
        }

        // Simulate handshake completion.
        handshake(channel->stopped() ? error::channel_stopped : error::success);
    }

    bool attached_handshake() const NOEXCEPT
    {
        return handshaked_;
    }

    void attach_protocols(const channel::ptr&) NOEXCEPT override
    {
        if (!protocoled_)
        {
            protocoled_ = true;
            require_protocoled_.set_value(true);
        }
    }

    bool attached_protocol() const NOEXCEPT
    {
        return protocoled_;
    }

    bool require_attached_protocol() const NOEXCEPT
    {
        return require_protocoled_.get_future().get();
    }

private:
    mutable bool handshaked_{ false };
    mutable bool protocoled_{ false };
    mutable std::promise<bool> require_protocoled_;
};

class mock_p2p
  : public p2p
{
public:
    using p2p::p2p;

    acceptor::ptr create_acceptor() NOEXCEPT override
    {
        ++acceptors_;
        return p2p::create_acceptor();
    }

    size_t acceptors() const NOEXCEPT
    {
        return acceptors_;
    }

    connector::ptr create_connector() NOEXCEPT override
    {
        ++connectors_;
        return p2p::create_connector();
    }

    size_t connectors() const NOEXCEPT
    {
        return connectors_;
    }

    void take(address_item_handler&& handler) NOEXCEPT override
    {
        handler(error::invalid_magic, {});
    }

    void fetch(address_handler&& handler) NOEXCEPT override
    {
        handler(error::bad_stream, {});
    }

    void restore(const address_item_cptr& address,
        result_handler&& complete) NOEXCEPT override
    {
        restored_ = *address;
        complete(error::invalid_magic);
    }

    const address_item& restored() const NOEXCEPT
    {
        return restored_;
    }

    void save(const address_cptr& message,
        count_handler&& complete) NOEXCEPT override
    {
        saveds_ = message->addresses;
        complete(error::bad_stream, zero);
    }

    const address_items& saveds() const NOEXCEPT
    {
        return saveds_;
    }

    uint64_t stored_nonce() const NOEXCEPT
    {
        return stored_;
    }

    uint64_t unstored_nonce() const NOEXCEPT
    {
        return unstored_;
    }

    bool stored_nonce_result() const NOEXCEPT
    {
        return stored_result_;
    }

    uint64_t counted_channel() const NOEXCEPT
    {
        return counted_;
    }

    uint64_t uncounted_channel() const NOEXCEPT
    {
        return uncounted_;
    }

    code counted_channel_result() const NOEXCEPT
    {
        return counted_result_;
    }

    session_seed::ptr attach_seed_session() NOEXCEPT override
    {
        return attach<mock_session_seed>(*this);
    }

protected:
    bool store_nonce(const channel& channel) NOEXCEPT override
    {
        stored_ = channel.nonce();
        return ((stored_result_ = p2p::store_nonce(channel)));
    }

    bool unstore_nonce(const channel& channel) NOEXCEPT override
    {
        unstored_ = channel.nonce();
        return p2p::unstore_nonce(channel);
    }

    code count_channel(const channel& channel) NOEXCEPT override
    {
        counted_ = channel.nonce();
        return ((counted_result_ = p2p::count_channel(channel)));
    }

    void uncount_channel(const channel& channel) NOEXCEPT override
    {
        uncounted_ = channel.nonce();
        p2p::uncount_channel(channel);
    }

private:
    size_t acceptors_{ 0 };
    size_t connectors_{ 0 };

    address_item restored_{};
    address_items saveds_{};

    uint64_t stored_{ 0 };
    uint64_t unstored_{ 0 };
    bool stored_result_{ false };

    uint64_t counted_{ 0 };
    uint64_t uncounted_{ 0 };
    code counted_result_{ error::invalid_magic };

    class mock_session_seed
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

// construct/settings

BOOST_AUTO_TEST_CASE(session__construct__always__expected_settings)
{
    const logger log{};
    constexpr auto expected = 42u;
    settings set(selection::mainnet);
    set.threads = expected;
    p2p net(set, log);
    mock_session session(net, 1);
    BOOST_REQUIRE_EQUAL(session.settings().threads, expected);
}

// properties

BOOST_AUTO_TEST_CASE(session__properties__default__expected)
{
    const logger log{};
    settings set(selection::mainnet);
    p2p net(set, log);
    mock_session session(net, 1);
    BOOST_REQUIRE(session.stopped());
    BOOST_REQUIRE(!session.stranded());
    BOOST_REQUIRE(is_zero(session.address_count()));
    BOOST_REQUIRE(is_zero(session.channel_count()));
    BOOST_REQUIRE(is_zero(session.inbound_channel_count()));
}

// factories

BOOST_AUTO_TEST_CASE(session__create_acceptor__always__expected)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);
    BOOST_REQUIRE(session.create_acceptor());
    BOOST_REQUIRE_EQUAL(net.acceptors(), 1u);
}

BOOST_AUTO_TEST_CASE(session__create_connector__always__expected)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);
    BOOST_REQUIRE(session.create_connector());
    BOOST_REQUIRE_EQUAL(net.connectors(), 1u);
}

BOOST_AUTO_TEST_CASE(session__create_connectors__always__expected)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);
    constexpr auto expected = 42u;
    const auto connectors = session.create_connectors(expected);
    BOOST_REQUIRE(connectors);
    BOOST_REQUIRE_EQUAL(connectors->size(), expected);
    BOOST_REQUIRE_EQUAL(net.connectors(), expected);
}

// utilities

BOOST_AUTO_TEST_CASE(session__take__always__calls_network)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);

    std::promise<code> taken;
    session.take([&](const code& ec, const address_item_cptr&) NOEXCEPT
    {
        taken.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(taken.get_future().get(), error::invalid_magic);
}

BOOST_AUTO_TEST_CASE(session__fetch__always__calls_network)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);

    std::promise<code> fetched;
    session.fetch([&](const code& ec, const address_cptr&) NOEXCEPT
    {
        fetched.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(fetched.get_future().get(), error::bad_stream);
}

BOOST_AUTO_TEST_CASE(session__restore__always__calls_network_with_expected_address)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);

    std::promise<code> save;
    const address_item item{ 42, 24, unspecified_ip_address, 4224u };
    session.restore(system::to_shared(item), [&](const code& ec) NOEXCEPT
    {
        save.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(save.get_future().get(), error::invalid_magic);

    const auto& saved = net.restored();
    BOOST_REQUIRE_EQUAL(saved.timestamp, 42u);
    BOOST_REQUIRE_EQUAL(saved.services, 24u);
    BOOST_REQUIRE_EQUAL(saved.ip, unspecified_ip_address);
    BOOST_REQUIRE_EQUAL(saved.port, 4224u);
}

BOOST_AUTO_TEST_CASE(session__save__always__calls_network_with_expected_addresses)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);

    std::promise<code> save;
    const address_items items{ {}, { 42, 24, unspecified_ip_address, 4224u } };
    session.save(system::to_shared(address{ items }), [&](const code& ec, auto) NOEXCEPT
    {
        save.set_value(ec);
    });

    BOOST_REQUIRE_EQUAL(save.get_future().get(), error::bad_stream);

    const auto& saveds = net.saveds();
    BOOST_REQUIRE_EQUAL(saveds.size(), 2u);
    BOOST_REQUIRE_EQUAL(saveds[1].timestamp, 42u);
    BOOST_REQUIRE_EQUAL(saveds[1].services, 24u);
    BOOST_REQUIRE_EQUAL(saveds[1].ip, unspecified_ip_address);
    BOOST_REQUIRE_EQUAL(saveds[1].port, 4224u);
}

// stop

BOOST_AUTO_TEST_CASE(session__stop__stopped__true)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);
    BOOST_REQUIRE(session.stopped());

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session.stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
}

// start

BOOST_AUTO_TEST_CASE(session__start__restart__operation_failed)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session.start([&](const code& ec)
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    std::promise<code> restarted;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session.start([&](const code& ec) NOEXCEPT
        {
            restarted.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(restarted.get_future().get(), error::operation_failed);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session.stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

BOOST_AUTO_TEST_CASE(session__start__stop__success)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    mock_session session(net, 1);

    std::promise<code> started;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session.start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [&]() NOEXCEPT
    {
        session.stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session.stopped());
}

// channel sequence

BOOST_AUTO_TEST_CASE(session__start_channel__session_not_started__handlers_service_stopped_channel_service_stopped_not_pent_or_stored)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    auto session = std::make_shared<mock_session>(net, 1);
    BOOST_REQUIRE(session->stopped());

    const auto socket = std::make_shared<network::socket>(net.log, net.service());
    const auto channel = std::make_shared<mock_channel>(net.log, socket, session->settings(), 42);

    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]() NOEXCEPT
    {
        session->start_channel(channel,
            [&](const code& ec) NOEXCEPT
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec) NOEXCEPT
            {
                stopped_channel.set_value(ec);
            });
    });

    // Channel stopped early due to session being stopped (not started).
    BOOST_REQUIRE(!session->attached_handshake());
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(!session->attached_handshake());
    BOOST_REQUIRE(!channel->resumed());
    BOOST_REQUIRE(!session->attached_protocol());
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::service_stopped);

    // Channel was not stored or counted.
    BOOST_REQUIRE(is_zero(net.stored_nonce()));
    BOOST_REQUIRE(is_zero(net.counted_channel()));

    // Channel was not unstored or uncounted.
    BOOST_REQUIRE(is_zero(net.unstored_nonce()));
    BOOST_REQUIRE(is_zero(net.uncounted_channel()));
}

BOOST_AUTO_TEST_CASE(session__start_channel__channel_not_started__handlers_channel_stopped_channel_channel_stopped_stored_and_not_counted)
{
    const logger log{};
    settings set(selection::mainnet);
    mock_p2p net(set, log);
    auto session = std::make_shared<mock_session>(net, 1);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.log, net.service());
    const auto channel = std::make_shared<mock_channel>(net.log, socket, session->settings(), 42);

    // Stop the channel (started by default).
    std::promise<bool> unstarted_channel;
    boost::asio::post(channel->strand(), [=, &unstarted_channel]() NOEXCEPT
    {
        channel->stopper(error::invalid_magic);
        unstarted_channel.set_value(true);
    });

    BOOST_REQUIRE(unstarted_channel.get_future().get());
    BOOST_REQUIRE(channel->stopped());

    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]() NOEXCEPT
    {
        session->start_channel(channel,
            [&](const code& ec) NOEXCEPT
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec) NOEXCEPT
            {
                stopped_channel.set_value(ec);
            });
    });

    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::channel_stopped);
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(channel->resumed());
    BOOST_REQUIRE(!session->attached_protocol());
    BOOST_REQUIRE(!channel->reresumed());
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::channel_stopped);
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::channel_stopped);

    // stored and not counted
    BOOST_REQUIRE(net.stored_nonce_result());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE(is_zero(net.counted_channel()));

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());

    // unstored and not counted/uncounted
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE(is_zero(net.counted_channel()));
    BOOST_REQUIRE(is_zero(net.uncounted_channel()));
}

BOOST_AUTO_TEST_CASE(session__start_channel__all_started__handlers_expected_channel_service_stopped_stored_and_counted)
{
    const logger log{};
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    mock_p2p net(set, log);
    auto session = std::make_shared<mock_session>(net, 1);

    std::promise<code> started_net;
    boost::asio::post(net.strand(), [&net, &started_net]() NOEXCEPT
    {
        net.start([&](const code& ec) NOEXCEPT
        {
            started_net.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started_net.get_future().get(), error::success);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.log, net.service());
    const auto channel = std::make_shared<mock_channel>(net.log, socket, session->settings(), 42);
    
    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]() NOEXCEPT
    {
        session->start_channel(channel,
            [&](const code& ec) NOEXCEPT
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec) NOEXCEPT
            {
                stopped_channel.set_value(ec);
            });
    });

    // Channel stopped by heading read fail (bad_stream), stop method called by session.
    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::success);
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(channel->resumed());
    ////BOOST_REQUIRE(session->attached_protocol());
    ////BOOST_REQUIRE(channel->reresumed());

    // Race between bad_stream and channel_stopped.
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE(channel->stop_code());

    // stored and counted
    BOOST_REQUIRE(net.stored_nonce_result());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.counted_channel(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.counted_channel_result(), error::success);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());

    // unstored and uncounted
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.uncounted_channel(), channel->nonce());
}

BOOST_AUTO_TEST_CASE(session__start_channel__outbound_all_started__handlers_expected_channel_success_stored_and_counted)
{
    const logger log{};
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    mock_p2p net(set, log);

    auto session = std::make_shared<mock_session>(net, 1);

    std::promise<code> started_net;
    boost::asio::post(net.strand(), [&net, &started_net]() NOEXCEPT
    {
        net.start([&](const code& ec) NOEXCEPT
        {
            started_net.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started_net.get_future().get(), error::success);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.log, net.service());
    const auto channel = std::make_shared<mock_channel_no_read>(net.log, socket, session->settings(), 42);
    
    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]() NOEXCEPT
    {
        session->start_channel(channel,
            [&](const code& ec) NOEXCEPT
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec) NOEXCEPT
            {
                stopped_channel.set_value(ec);
            });
    });

    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::success);
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(channel->resumed());
    BOOST_REQUIRE(session->require_attached_protocol());
    BOOST_REQUIRE(!channel->stopped());

    // stored and counted
    BOOST_REQUIRE(net.stored_nonce_result());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.counted_channel(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.counted_channel_result(), error::success);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(channel->reresumed());
    BOOST_REQUIRE(channel->stopped());

    net.close();
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::service_stopped);

    // unstored and uncounted
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.uncounted_channel(), channel->nonce());
}

BOOST_AUTO_TEST_CASE(session__start_channel__inbound_all_started__handlers_expected_channel_success_not_stored_and_counted)
{
    const logger log{};
    settings set(selection::mainnet);
    set.host_pool_capacity = 0;
    mock_p2p net(set, log);

    auto session = std::make_shared<mock_session>(net, 1);

    std::promise<code> started_net;
    boost::asio::post(net.strand(), [&net, &started_net]() NOEXCEPT
    {
        net.start([&](const code& ec) NOEXCEPT
        {
            started_net.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started_net.get_future().get(), error::success);

    std::promise<code> started;
    boost::asio::post(net.strand(), [=, &started]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);

    const auto socket = std::make_shared<network::socket>(net.log, net.service());
    const auto channel = std::make_shared<mock_channel_no_read>(net.log, socket, session->settings(), 42);
    
    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(net.strand(), [=, &started_channel, &stopped_channel]() NOEXCEPT
    {
        session->start_channel(channel,
            [&](const code& ec) NOEXCEPT
            {
                started_channel.set_value(ec);
            },
            [&](const code& ec) NOEXCEPT
            {
                stopped_channel.set_value(ec);
            });
    });

    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::success);
    BOOST_REQUIRE(session->attached_handshake());
    BOOST_REQUIRE(channel->resumed());
    BOOST_REQUIRE(session->require_attached_protocol());
    BOOST_REQUIRE(!channel->stopped());

    // stored and counted
    BOOST_REQUIRE(net.stored_nonce_result());
    BOOST_REQUIRE_EQUAL(net.stored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.counted_channel(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.counted_channel_result(), error::success);

    std::promise<bool> stopped;
    boost::asio::post(net.strand(), [=, &stopped]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
    BOOST_REQUIRE(session->stopped());
    BOOST_REQUIRE(channel->reresumed());
    BOOST_REQUIRE(channel->stopped());

    net.close();
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(channel->stopped());
    BOOST_REQUIRE_EQUAL(channel->stop_code(), error::service_stopped);

    // unstored and uncounted
    BOOST_REQUIRE_EQUAL(net.unstored_nonce(), channel->nonce());
    BOOST_REQUIRE_EQUAL(net.uncounted_channel(), channel->nonce());
}

BOOST_AUTO_TEST_SUITE_END()
