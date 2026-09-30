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
#include <future>
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(acceptor_tests)

class accessor
  : public acceptor
{
public:
    using acceptor::acceptor;

    const asio::context& get_service() const NOEXCEPT
    {
        return service_;
    }

    const asio::strand& get_strand() const NOEXCEPT
    {
        return strand_;
    }

    const asio::acceptor& get_acceptor() const NOEXCEPT
    {
        return acceptor_;
    }

    size_t get_maximum_request() const NOEXCEPT
    {
        return parameters_.maximum_request;
    }

    bool get_stopped() const NOEXCEPT
    {
        return stopped_;
    }
};

// TODO: increase test coverage.

BOOST_AUTO_TEST_CASE(acceptor__construct__default__stopped_expected)
{
    const logger log{};
    threadpool pool(1);
    constexpr auto maximum = 42u;
    std::atomic_bool suspended{ false };
    asio::strand strand(pool.service().get_executor());
    acceptor::parameters params{ .maximum_request = maximum,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, std::move(params));

    BOOST_REQUIRE(&instance->get_service() == &pool.service());
    BOOST_REQUIRE(&instance->get_strand() == &strand);
    BOOST_REQUIRE(!instance->get_acceptor().is_open());
    BOOST_REQUIRE(instance->get_stopped());
    BOOST_REQUIRE_EQUAL(instance->get_maximum_request(), maximum);
}

// TODO: There is no way to fake failures in start.
BOOST_AUTO_TEST_CASE(acceptor__start__stop__success)
{
    const logger log{};
    threadpool pool(1);
    std::atomic_bool suspended{ false };
    asio::strand strand(pool.service().get_executor());
    acceptor::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, std::move(params));

    // Result codes inconsistent due to context.
    instance->start(messages::peer::address_item{ 0, 0, messages::peer::ipv6_t{}, 42 });

    boost::asio::post(strand, [instance]() NOEXCEPT
    {
        instance->stop();
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(instance->get_stopped());
}

// race
BOOST_AUTO_TEST_CASE(acceptor__accept__stop_suspended__service_stopped_or_suspended)
{
    // TODO: There is no way to fake successful acceptance.
    const logger log{};
    threadpool pool(2);
    std::atomic_bool suspended{ true };
    asio::strand strand(pool.service().get_executor());
    acceptor::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, std::move(params));

    // Result codes inconsistent due to context.
    instance->start(messages::peer::address_item{ 0, 0, messages::peer::ipv6_t{}, 42 });

    std::pair<code, socket::ptr>  result{};
    boost::asio::post(strand, [&, instance]() NOEXCEPT
    {
        instance->accept([&](const code& ec, const socket::ptr& socket) NOEXCEPT
        {
            result.first = ec;
            result.second = socket;
        });

        std::this_thread::sleep_for(microseconds(1));
        instance->stop();
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(instance->get_stopped());
    BOOST_REQUIRE(result.first == error::service_suspended || result.first == error::service_stopped);
    BOOST_REQUIRE(!result.second);
}

BOOST_AUTO_TEST_CASE(acceptor__accept__stop__channel_stopped)
{
    // TODO: There is no way to fake successful acceptance.
    const logger log{};
    threadpool pool(2);
    std::atomic_bool suspended{ false };
    asio::strand strand(pool.service().get_executor());
    acceptor::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, std::move(params));

    // Result codes inconsistent due to context.
    instance->start(messages::peer::address_item{ 0, 0, messages::peer::ipv6_t{}, 42 });

    std::pair<code, socket::ptr>  result{};
    boost::asio::post(strand, [&, instance]() NOEXCEPT
    {
        instance->accept([&](const code& ec, const socket::ptr& socket) NOEXCEPT
        {
            result.first = ec;
            result.second = socket;
        });

        std::this_thread::sleep_for(microseconds(1));
        instance->stop();
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());
    BOOST_REQUIRE(instance->get_stopped());
    BOOST_REQUIRE(result.first);
    BOOST_REQUIRE(!result.second);
}

struct acceptor_setup_fixture
{
    DELETE_COPY_MOVE(acceptor_setup_fixture);

    acceptor_setup_fixture()
      : pool(2), strand(pool.service().get_executor()), client(context)
    {
        log.stop();
        acceptor::parameters params{ .maximum_request = 42, .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
        instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, std::move(params));
    }

    ~acceptor_setup_fixture()
    {
        std::promise<bool> stopped{};
        boost::asio::post(strand, [this, &stopped]() NOEXCEPT
        {
            instance->stop();
            stopped.set_value(true);
        });

        stopped.get_future().get();
        client.close();
        pool.stop();
        pool.join();
    }

    code start(const config::authority& local)
    {
        std::promise<code> started{};
        boost::asio::post(strand, [this, &local, &started]() NOEXCEPT
        {
            started.set_value(instance->start(local));
        });

        return started.get_future().get();
    }

    uint16_t port()
    {
        std::promise<uint16_t> promise{};
        boost::asio::post(strand, [this, &promise]() NOEXCEPT
        {
            promise.set_value(instance->local().port());
        });

        return promise.get_future().get();
    }

    std::future<code> accept()
    {
        boost::asio::post(strand, [this]() NOEXCEPT
        {
            instance->accept([this](const code& ec, const socket::ptr& socket) NOEXCEPT
            {
                if (socket)
                {
                    inbound = socket->inbound();
                    socket->stop();
                }

                accepted.set_value(ec);
            });
        });

        return accepted.get_future();
    }

    void connect()
    {
        client.connect({ boost::asio::ip::address_v4::loopback(), port() });
    }

    logger log{};
    threadpool pool;
    std::atomic_bool suspended{ false };
    asio::strand strand;
    std::shared_ptr<accessor> instance{};
    boost::asio::io_context context{};
    boost::asio::ip::tcp::socket client;
    std::promise<code> accepted{};
    bool inbound{};
};

BOOST_FIXTURE_TEST_CASE(acceptor__start__started__operation_failed, acceptor_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(start({ boost::asio::ip::address_v4::loopback(), 0 }), error::success);
    BOOST_REQUIRE_EQUAL(start({ boost::asio::ip::address_v4::loopback(), 0 }), error::operation_failed);
}

BOOST_FIXTURE_TEST_CASE(acceptor__start__unassigned_address__failure_stopped, acceptor_setup_fixture)
{
    BOOST_REQUIRE(start({ boost::asio::ip::make_address_v4("1.2.3.4"), 0 }));
    BOOST_REQUIRE(instance->get_stopped());
}

BOOST_FIXTURE_TEST_CASE(acceptor__accept__connected__success_inbound, acceptor_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(start({ boost::asio::ip::address_v4::loopback(), 0 }), error::success);
    auto result = accept();
    connect();
    BOOST_REQUIRE_EQUAL(result.get(), error::success);
    BOOST_REQUIRE(inbound);
}

BOOST_FIXTURE_TEST_CASE(acceptor__local__started__bound_port, acceptor_setup_fixture)
{
    BOOST_REQUIRE_EQUAL(start({ boost::asio::ip::address_v4::loopback(), 0 }), error::success);
    BOOST_REQUIRE_NE(port(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
