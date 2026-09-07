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
#include <cstdlib>
#include <future>
#include "../test.hpp"

// Servers for the pyzmq role harness (test/pyzmq/zmtp_roles.py), which runs
// each case by name against a real libzmq peer with ZMTP_HARNESS set. The
// cases pass trivially otherwise, so a full run never waits on a peer.
BOOST_AUTO_TEST_SUITE(zmtp_harness_tests, *boost::unit_test::disabled())

using context = network::zmtp::context;
using role = network::zmtp::role;
using system::data_chunk;

constexpr uint16_t puller_port = 65031;
constexpr uint16_t replier_port = 65032;
constexpr uint16_t router_port = 65033;
constexpr auto patience = seconds(10);

// A server socket in the given role, accepted from the harness peer.
// Completion promises are shared so that a late completion after a failed
// wait does not touch a destroyed promise.
struct harness_fixture
{
    harness_fixture(role value, uint16_t port)
      : enabled(!is_null(std::getenv("ZMTP_HARNESS"))),
        pool(1),
        params
        {
            .maximum_request = 4096,
            .context = socket::context{ std::cref(configuration) },
            .role = value
        },
        sock(std::make_shared<network::socket>(log, pool.service(), params)),
        strand(pool.service().get_executor()),
        acceptor(strand)
    {
        if (!enabled)
            return;

        boost_code ec{};
        acceptor.open(asio::tcp::v4(), ec);
        BOOST_REQUIRE(!ec);
        acceptor.set_option(asio::reuse_address(true), ec);
        BOOST_REQUIRE(!ec);
        acceptor.bind({ boost::asio::ip::address_v4::loopback(), port }, ec);
        BOOST_REQUIRE(!ec);
        acceptor.listen(1, ec);
        BOOST_REQUIRE(!ec);

        const auto promised = std::make_shared<std::promise<code>>();
        sock->accept(acceptor, [promised](const code& result) NOEXCEPT
        {
            promised->set_value(result);
        });

        auto accepted = promised->get_future();
        BOOST_REQUIRE(accepted.wait_for(patience) == std::future_status::ready);
        BOOST_REQUIRE_EQUAL(accepted.get(), error::success);
    }

    ~harness_fixture()
    {
        sock->stop();
        pool.stop();
        BOOST_REQUIRE(pool.join());
    }

    code read(rpc::request& request)
    {
        const auto got = std::make_shared<std::promise<code>>();
        sock->rpc_read(buffer, request, [got](const code& ec, size_t) NOEXCEPT
        {
            got->set_value(ec);
        });

        auto pending = got->get_future();
        BOOST_REQUIRE(pending.wait_for(patience) == std::future_status::ready);
        return pending.get();
    }

    code notify(rpc::request&& notification)
    {
        const auto sent = std::make_shared<std::promise<code>>();
        sock->rpc_notify(std::move(notification),
            [sent](const code& ec, size_t) NOEXCEPT
            {
                sent->set_value(ec);
            });

        auto pending = sent->get_future();
        BOOST_REQUIRE(pending.wait_for(patience) == std::future_status::ready);
        return pending.get();
    }

    code respond(rpc::response&& response)
    {
        const auto sent = std::make_shared<std::promise<code>>();
        sock->rpc_write(std::move(response),
            [sent](const code& ec, size_t) NOEXCEPT
            {
                sent->set_value(ec);
            });

        auto pending = sent->get_future();
        BOOST_REQUIRE(pending.wait_for(patience) == std::future_status::ready);
        return pending.get();
    }

    const bool enabled;
    const logger log{};
    threadpool pool;
    const context configuration{};
    socket::parameters params;
    socket::ptr sock;
    asio::strand strand;
    asio::acceptor acceptor;
    http::flat_buffer buffer{};
};

struct puller_fixture
  : harness_fixture
{
    puller_fixture() : harness_fixture(role::puller, puller_port) {}
};

struct replier_fixture
  : harness_fixture
{
    replier_fixture() : harness_fixture(role::replier, replier_port) {}
};

struct router_fixture
  : harness_fixture
{
    router_fixture() : harness_fixture(role::router, router_port) {}
};

static data_chunk param_of(const rpc::request& request, size_t index)
{
    const auto& params = std::get<rpc::array_t>(*request.message.params);
    const auto& any = std::get<rpc::any_t>(params.at(index).value());
    return *any.as<const data_chunk>();
}

static size_t params_of(const rpc::request& request)
{
    return std::get<rpc::array_t>(*request.message.params).size();
}

static data_chunk chunk(const std::string& text)
{
    return system::to_chunk(text);
}

// PUSH peer sends three [push][one][two] messages.
BOOST_FIXTURE_TEST_CASE(zmtp_harness__puller, puller_fixture)
{
    if (!enabled)
        return;

    for (size_t count{}; count < 3; ++count)
    {
        rpc::request request{};
        BOOST_REQUIRE_EQUAL(read(request), error::success);
        BOOST_REQUIRE_EQUAL(request.message.method, "push");
        BOOST_REQUIRE_EQUAL(params_of(request), 2u);
        BOOST_REQUIRE_EQUAL(param_of(request, 0), chunk("one"));
        BOOST_REQUIRE_EQUAL(param_of(request, 1), chunk("two"));
    }
}

// REQ peer sends [echo][text] twice (answered with [text]) then [fail]
// (answered with an error, code and message).
BOOST_FIXTURE_TEST_CASE(zmtp_harness__replier, replier_fixture)
{
    if (!enabled)
        return;

    for (size_t count{}; count < 2; ++count)
    {
        rpc::request request{};
        BOOST_REQUIRE_EQUAL(read(request), error::success);
        BOOST_REQUIRE_EQUAL(request.message.method, "echo");
        BOOST_REQUIRE_EQUAL(params_of(request), 1u);

        rpc::response response{};
        response.message.result = rpc::value_t
        {
            rpc::any_t{ system::to_shared(param_of(request, 0)) }
        };

        BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);
    }

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "fail");

    rpc::response response{};
    response.message.error = rpc::result_t{ .code = -1, .message = "nope" };
    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);
}

// DEALER peer (routing id "peer1", REQ envelope) sends [][echo][text],
// answered with [][text] and then notified with [][topic][note], then sends
// [][done].
BOOST_FIXTURE_TEST_CASE(zmtp_harness__router, router_fixture)
{
    if (!enabled)
        return;

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE(request.message.id);
    BOOST_REQUIRE_EQUAL(std::get<rpc::string_t>(*request.message.id), "peer1");
    BOOST_REQUIRE_EQUAL(request.message.method, "echo");
    BOOST_REQUIRE_EQUAL(params_of(request), 1u);
    const auto identity = std::get<rpc::string_t>(*request.message.id);

    rpc::response response{};
    response.message.id = identity;
    response.message.result = rpc::value_t
    {
        rpc::any_t{ system::to_shared(param_of(request, 0)) }
    };

    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);

    rpc::request notification{};
    notification.message.id = identity;
    notification.message.method = "topic";
    notification.message.params = rpc::array_t
    {
        rpc::any_t{ system::to_shared(chunk("note")) }
    };

    BOOST_REQUIRE_EQUAL(notify(std::move(notification)), error::success);

    rpc::request last{};
    BOOST_REQUIRE_EQUAL(read(last), error::success);
    BOOST_REQUIRE_EQUAL(std::get<rpc::string_t>(*last.message.id), identity);
    BOOST_REQUIRE_EQUAL(last.message.method, "done");
}

BOOST_AUTO_TEST_SUITE_END()
