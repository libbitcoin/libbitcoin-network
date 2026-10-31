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

BC_AUTO_TEST_SUITE(acceptor_tests)

class accessor
  : public acceptor
{
public:
    using acceptor::acceptor;

    const io::context& get_service() const NOEXCEPT
    {
        return service_;
    }

    const io::strand& get_strand() const NOEXCEPT
    {
        return strand_;
    }

    const io::acceptor& get_acceptor() const NOEXCEPT
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

BC_AUTO_TEST_CASE(acceptor__construct__default__stopped_expected)
{
    const logger log{};
    threadpool pool(1);
    constexpr auto maximum = 42u;
    std::atomic_bool suspended{ false };
    io::strand strand(pool.service().get_executor());
    acceptor::parameters params{ .maximum_request = maximum,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, std::move(params));

    BC_REQUIRE(&instance->get_service() == &pool.service());
    BC_REQUIRE(&instance->get_strand() == &strand);
    BC_REQUIRE(!instance->get_acceptor().is_open());
    BC_REQUIRE(instance->get_stopped());
    BC_REQUIRE_EQUAL(instance->get_maximum_request(), maximum);
}

// TODO: There is no way to fake failures in start.
BC_AUTO_TEST_CASE(acceptor__start__stop__success)
{
    const logger log{};
    threadpool pool(1);
    std::atomic_bool suspended{ false };
    io::strand strand(pool.service().get_executor());
    acceptor::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, std::move(params));

    // Result codes inconsistent due to context.
    instance->start(messages::peer::address_item{ 0, 0, messages::peer::ipv6_t{}, 42 });

    io::post(strand, [instance]() NOEXCEPT
    {
        instance->stop();
    });

    pool.stop();
    BC_REQUIRE(pool.join());
    BC_REQUIRE(instance->get_stopped());
}

// race
BC_AUTO_TEST_CASE(acceptor__accept__stop_suspended__service_stopped_or_suspended)
{
    // TODO: There is no way to fake successful acceptance.
    const logger log{};
    threadpool pool(2);
    std::atomic_bool suspended{ true };
    io::strand strand(pool.service().get_executor());
    acceptor::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, std::move(params));

    // Result codes inconsistent due to context.
    instance->start(messages::peer::address_item{ 0, 0, messages::peer::ipv6_t{}, 42 });

    std::pair<code, socket::ptr>  result{};
    io::post(strand, [&, instance]() NOEXCEPT
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
    BC_REQUIRE(pool.join());
    BC_REQUIRE(instance->get_stopped());
    BC_REQUIRE(result.first == error::service_suspended || result.first == error::service_stopped);
    BC_REQUIRE(!result.second);
}

BC_AUTO_TEST_CASE(acceptor__accept__stop__channel_stopped)
{
    // TODO: There is no way to fake successful acceptance.
    const logger log{};
    threadpool pool(2);
    std::atomic_bool suspended{ false };
    io::strand strand(pool.service().get_executor());
    acceptor::parameters params{ .maximum_request = 42,
        .maximum_buffer = settings::tcp_server{ "test" }.maximum_buffer };
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), suspended, std::move(params));

    // Result codes inconsistent due to context.
    instance->start(messages::peer::address_item{ 0, 0, messages::peer::ipv6_t{}, 42 });

    std::pair<code, socket::ptr>  result{};
    io::post(strand, [&, instance]() NOEXCEPT
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
    BC_REQUIRE(pool.join());
    BC_REQUIRE(instance->get_stopped());
    BC_REQUIRE(result.first);
    BC_REQUIRE(!result.second);
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
        io::post(strand, [this, &stopped]() NOEXCEPT
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
        io::post(strand, [this, &local, &started]() NOEXCEPT
        {
            started.set_value(instance->start(local));
        });

        return started.get_future().get();
    }

    uint16_t port()
    {
        std::promise<uint16_t> promise{};
        io::post(strand, [this, &promise]() NOEXCEPT
        {
            promise.set_value(instance->local().port());
        });

        return promise.get_future().get();
    }

    std::future<code> accept()
    {
        io::post(strand, [this]() NOEXCEPT
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
        REQUIRE_SUCCESS(client, connect, test::loopback(port()));
    }

    logger log{};
    threadpool pool;
    std::atomic_bool suspended{ false };
    io::strand strand;
    std::shared_ptr<accessor> instance{};
    io::context context{};
    io::socket client;
    std::promise<code> accepted{};
    bool inbound{};
};

BC_FIXTURE_TEST_CASE(acceptor__start__started__operation_failed, acceptor_setup_fixture)
{
    BC_REQUIRE_EQUAL(start({ system::ip::ipv4::loopback(), 0 }), error::success);
    BC_REQUIRE_EQUAL(start({ system::ip::ipv4::loopback(), 0 }), error::operation_failed);
}

BC_FIXTURE_TEST_CASE(acceptor__start__unassigned_address__failure_stopped, acceptor_setup_fixture)
{
    const system::ip::ipv4 unassigned{ { 1, 2, 3, 4 } };
    BC_REQUIRE(start({ unassigned, 0 }));
    BC_REQUIRE(instance->get_stopped());
}

BC_FIXTURE_TEST_CASE(acceptor__accept__connected__success_inbound, acceptor_setup_fixture)
{
    BC_REQUIRE_EQUAL(start({ system::ip::ipv4::loopback(), 0 }), error::success);
    auto result = accept();
    connect();
    BC_REQUIRE_EQUAL(result.get(), error::success);
    BC_REQUIRE(inbound);
}

BC_FIXTURE_TEST_CASE(acceptor__local__started__bound_port, acceptor_setup_fixture)
{
    BC_REQUIRE_EQUAL(start({ system::ip::ipv4::loopback(), 0 }), error::success);
    BC_REQUIRE_NE(port(), 0u);
}

BC_AUTO_TEST_SUITE_END()
