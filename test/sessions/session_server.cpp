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

BOOST_AUTO_TEST_SUITE(session_server_tests)

using namespace bc::system::chain;

static const settings::secure_server server_options{ "test" };

class mock_channel
  : public channel
{
public:
    mock_channel(const logger& log, const socket::ptr& socket, uint64_t identifier, const settings_t& settings, const options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options)
    {
    }

    void resume() NOEXCEPT override
    {
        resumed_ = true;
    }

    bool resumed() const NOEXCEPT
    {
        return resumed_;
    }

private:
    std::atomic_bool resumed_{ false };
};

class mock_session
  : public session_server
{
public:
    mock_session(net& network, uint64_t identifier) NOEXCEPT
      : session_server(network, identifier, server_options)
    {
    }

    using session::start_channel;
    using session::defer;
    using session::create_service;
    using session::create_acceptor_sam;
    using session::stopped;

    void start_base(result_handler&& handler) NOEXCEPT
    {
        session::start(std::move(handler));
    }

    void attach_protocols(const channel::ptr&) NOEXCEPT override
    {
        protocoled_ = true;
    }

    channel::ptr create_channel(const socket::ptr&) NOEXCEPT override
    {
        return {};
    }

    bool attached_protocols() const NOEXCEPT
    {
        return protocoled_;
    }

private:
    std::atomic_bool protocoled_{ false };
};

class mock_session_handshake_fail
  : public mock_session
{
public:
    using mock_session::mock_session;

    void do_attach_handshake(const channel::ptr&, const result_handler& handshake) NOEXCEPT override
    {
        handshake(error::invalid_magic);
    }
};

template <class Session>
static void start(net& instance, const std::shared_ptr<Session>& session)
{
    std::promise<code> started;
    boost::asio::post(instance.strand(), [&]() NOEXCEPT
    {
        session->start_base([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
}

template <class Session>
static void stop(net& instance, const std::shared_ptr<Session>& session)
{
    std::promise<bool> stopped;
    boost::asio::post(instance.strand(), [&]() NOEXCEPT
    {
        session->stop();
        stopped.set_value(true);
    });

    BOOST_REQUIRE(stopped.get_future().get());
}

static std::shared_ptr<mock_channel> make_channel(net& instance)
{
    socket::parameters params{ .maximum_request = 42, .maximum_buffer = server_options.maximum_buffer };
    const auto socket = std::make_shared<network::socket>(instance.log, instance.service(), std::move(params));
    return std::make_shared<mock_channel>(instance.log, socket, 42, instance.network_settings(), server_options);
}

BOOST_AUTO_TEST_CASE(session_server__identifier__constructed__expected)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_session>(instance, 42);
    BOOST_REQUIRE_EQUAL(session->identifier(), 42u);
    instance.close();
}

BOOST_AUTO_TEST_CASE(session_server__start__disabled__success_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_session>(instance, 1);

    std::promise<code> started;
    boost::asio::post(instance.strand(), [&]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(started.get_future().get(), error::success);
    BOOST_REQUIRE(session->stopped());
    instance.close();
}

BOOST_AUTO_TEST_CASE(session_server__create_service__always__acceptor)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_session>(instance, 1);
    BOOST_REQUIRE(session->create_service({ .maximum_request = 42, .maximum_buffer = server_options.maximum_buffer }));
    BOOST_REQUIRE(session->create_acceptor_sam());
    instance.close();
}

BOOST_AUTO_TEST_CASE(session_server__defer__stopped__service_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_session>(instance, 1);

    std::promise<code> deferred;
    boost::asio::post(instance.strand(), [&]() NOEXCEPT
    {
        session->defer([&](const code& ec) NOEXCEPT
        {
            deferred.set_value(ec);
        });
    });

    BOOST_REQUIRE_EQUAL(deferred.get_future().get(), error::service_stopped);
    instance.close();
}

BOOST_AUTO_TEST_CASE(session_server__start_channel__started__protocols_attached_stopped_by_session)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_session>(instance, 1);
    start(instance, session);

    const auto channel = make_channel(instance);
    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(instance.strand(), [&]() NOEXCEPT
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
    BOOST_REQUIRE(session->attached_protocols());
    BOOST_REQUIRE(channel->resumed());
    BOOST_REQUIRE(!channel->stopped());

    stop(instance, session);
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::service_stopped);
    BOOST_REQUIRE(channel->stopped());
    instance.close();
}

BOOST_AUTO_TEST_CASE(session_server__start_channel__handshake_failure__handlers_failure_channel_stopped)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_session_handshake_fail>(instance, 1);
    start(instance, session);

    const auto channel = make_channel(instance);
    std::promise<code> started_channel;
    std::promise<code> stopped_channel;
    boost::asio::post(instance.strand(), [&]() NOEXCEPT
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

    BOOST_REQUIRE_EQUAL(started_channel.get_future().get(), error::invalid_magic);
    BOOST_REQUIRE_EQUAL(stopped_channel.get_future().get(), error::invalid_magic);
    BOOST_REQUIRE(!session->attached_protocols());
    BOOST_REQUIRE(channel->stopped());

    stop(instance, session);
    instance.close();
}

// Accept cycle (the test connects as a raw client on loopback).
// ============================================================================

static constexpr uint16_t server_port = 65156;

static settings::secure_server enabled_options()
{
    settings::secure_server options{ "test" };
    options.connections = 1;
    options.binds.emplace_back("127.0.0.1:65156");
    return options;
}

static const auto one_connection = enabled_options();

class mock_server
  : public session_server
{
public:
    mock_server(net& network, uint64_t identifier, const options_t& options) NOEXCEPT
      : session_server(network, identifier, options), options_(options)
    {
    }

    void attach_protocols(const channel::ptr&) NOEXCEPT override
    {
        if (!protocoled_.exchange(true))
            attached_.set_value(true);
    }

    channel::ptr create_channel(const socket::ptr& socket) NOEXCEPT override
    {
        return std::make_shared<mock_channel>(log, socket, create_key(), network_settings(), options_);
    }

    bool require_attached() NOEXCEPT
    {
        return attached_.get_future().get();
    }

private:
    const options_t& options_;
    std::atomic_bool protocoled_{ false };
    std::promise<bool> attached_{};
};

class mock_server_disabled
  : public mock_server
{
public:
    using mock_server::mock_server;

    bool enabled() const NOEXCEPT override
    {
        return false;
    }
};

template <class Session>
static code start_server(net& instance, const std::shared_ptr<Session>& session)
{
    std::promise<code> started;
    boost::asio::post(instance.strand(), [&]() NOEXCEPT
    {
        session->start([&](const code& ec) NOEXCEPT
        {
            started.set_value(ec);
        });
    });

    return started.get_future().get();
}

// Read until the server disconnects, returning true if dropped.
static bool dropped(boost::asio::ip::tcp::socket& client)
{
    boost_code ec{};
    uint8_t byte{};
    boost::asio::read(client, boost::asio::buffer(&byte, one), ec);
    return ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset;
}

BOOST_AUTO_TEST_CASE(session_server__accept__client__protocols_attached)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_server>(instance, 1, one_connection);
    BOOST_REQUIRE_EQUAL(start_server(instance, session), error::success);

    boost::asio::io_context io{};
    boost::asio::ip::tcp::socket client{ io };
    client.connect({ boost::asio::ip::address_v4::loopback(), server_port });
    BOOST_REQUIRE(session->require_attached());

    stop(instance, session);
    BOOST_REQUIRE(dropped(client));
    instance.close();
}

BOOST_AUTO_TEST_CASE(session_server__accept__oversubscribed__second_client_dropped)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_server>(instance, 1, one_connection);
    BOOST_REQUIRE_EQUAL(start_server(instance, session), error::success);

    boost::asio::io_context io{};
    boost::asio::ip::tcp::socket first{ io };
    first.connect({ boost::asio::ip::address_v4::loopback(), server_port });
    BOOST_REQUIRE(session->require_attached());

    boost::asio::ip::tcp::socket second{ io };
    second.connect({ boost::asio::ip::address_v4::loopback(), server_port });
    BOOST_REQUIRE(dropped(second));

    stop(instance, session);
    instance.close();
}

BOOST_AUTO_TEST_CASE(session_server__accept__disabled__client_dropped)
{
    const logger log{};
    settings set(selection::mainnet);
    net instance(set, log);
    const auto session = std::make_shared<mock_server_disabled>(instance, 1, one_connection);
    BOOST_REQUIRE_EQUAL(start_server(instance, session), error::success);

    boost::asio::io_context io{};
    boost::asio::ip::tcp::socket client{ io };
    client.connect({ boost::asio::ip::address_v4::loopback(), server_port });
    BOOST_REQUIRE(dropped(client));

    stop(instance, session);
    instance.close();
}

BOOST_AUTO_TEST_CASE(session_server__accept__blacklisted__client_dropped)
{
    const logger log{};
    settings set(selection::mainnet);
    set.blacklists.emplace_back("127.0.0.1");
    net instance(set, log);
    const auto session = std::make_shared<mock_server>(instance, 1, one_connection);
    BOOST_REQUIRE_EQUAL(start_server(instance, session), error::success);

    boost::asio::io_context io{};
    boost::asio::ip::tcp::socket client{ io };
    client.connect({ boost::asio::ip::address_v4::loopback(), server_port });
    BOOST_REQUIRE(dropped(client));

    stop(instance, session);
    instance.close();
}

BOOST_AUTO_TEST_CASE(session_server__accept__not_whitelisted__client_dropped)
{
    const logger log{};
    settings set(selection::mainnet);
    set.whitelists.emplace_back("1.2.3.4");
    net instance(set, log);
    const auto session = std::make_shared<mock_server>(instance, 1, one_connection);
    BOOST_REQUIRE_EQUAL(start_server(instance, session), error::success);

    boost::asio::io_context io{};
    boost::asio::ip::tcp::socket client{ io };
    client.connect({ boost::asio::ip::address_v4::loopback(), server_port });
    BOOST_REQUIRE(dropped(client));

    stop(instance, session);
    instance.close();
}

BOOST_AUTO_TEST_SUITE_END()
