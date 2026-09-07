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
#include "zmtp_setup_fixture.hpp"

// Servers for the pyzmq role harness (test/pyzmq/zmtp_roles.py), which runs
// each suite by name against a real libzmq peer with ZMTP_HARNESS set. The
// cases pass trivially otherwise, so a full run never waits on a peer.

// socket_type/compatible.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_SUITE(zmtp_role_tests)

BOOST_AUTO_TEST_CASE(zmtp_role__socket_type__each_role__expected)
{
    BOOST_REQUIRE_EQUAL(socket_type(zmtp_role::publisher), "PUB");
    BOOST_REQUIRE_EQUAL(socket_type(zmtp_role::puller), "PULL");
    BOOST_REQUIRE_EQUAL(socket_type(zmtp_role::replier), "REP");
    BOOST_REQUIRE_EQUAL(socket_type(zmtp_role::router), "ROUTER");
    BOOST_REQUIRE(socket_type(zmtp_role::undefined).empty());
}

BOOST_AUTO_TEST_CASE(zmtp_role__compatible__peer_socket_types__expected)
{
    BOOST_REQUIRE(compatible(zmtp_role::publisher, "SUB"));
    BOOST_REQUIRE(compatible(zmtp_role::publisher, "XSUB"));
    BOOST_REQUIRE(!compatible(zmtp_role::publisher, "PUSH"));

    BOOST_REQUIRE(compatible(zmtp_role::puller, "PUSH"));
    BOOST_REQUIRE(!compatible(zmtp_role::puller, "SUB"));

    BOOST_REQUIRE(compatible(zmtp_role::replier, "REQ"));
    BOOST_REQUIRE(compatible(zmtp_role::replier, "DEALER"));
    BOOST_REQUIRE(!compatible(zmtp_role::replier, "ROUTER"));

    BOOST_REQUIRE(compatible(zmtp_role::router, "REQ"));
    BOOST_REQUIRE(compatible(zmtp_role::router, "DEALER"));
    BOOST_REQUIRE(compatible(zmtp_role::router, "ROUTER"));

    BOOST_REQUIRE(!compatible(zmtp_role::undefined, "SUB"));
}

BOOST_AUTO_TEST_SUITE_END()

// PUSH peer sends three [push][one][two] messages.
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_role_puller_tests, role_puller_fixture, *boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(zmtp_role__puller__push_peer__expected)
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

BOOST_AUTO_TEST_SUITE_END()

// REQ peer sends [echo][text] twice (answered with [text]) then [fail]
// (answered with an error, code and message).
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_role_replier_tests, role_replier_fixture, *boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(zmtp_role__replier__req_peer__expected)
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
        response.message.result = rpc::value_t{ rpc::any_t{ system::to_shared(param_of(request, 0)) } };
        BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);
    }

    rpc::request request{};
    BOOST_REQUIRE_EQUAL(read(request), error::success);
    BOOST_REQUIRE_EQUAL(request.message.method, "fail");

    rpc::response response{};
    response.message.error = rpc::result_t{ .code = -1, .message = "nope" };
    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);
}

BOOST_AUTO_TEST_SUITE_END()

// DEALER peer (routing id "peer1", REQ envelope) sends [][echo][text],
// answered with [][text] and then notified with [][topic][note], then sends
// [][done].
// ----------------------------------------------------------------------------

BOOST_FIXTURE_TEST_SUITE(zmtp_role_router_tests, role_router_fixture, *boost::unit_test::disabled())

BOOST_AUTO_TEST_CASE(zmtp_role__router__dealer_peer__expected)
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
    response.message.result = rpc::value_t{ rpc::any_t{ system::to_shared(param_of(request, 0)) } };
    BOOST_REQUIRE_EQUAL(respond(std::move(response)), error::success);

    rpc::request notification{};
    notification.message.id = identity;
    notification.message.method = "topic";
    notification.message.params = rpc::array_t{ rpc::any_t{ system::to_shared(chunk("note")) } };
    BOOST_REQUIRE_EQUAL(notify(std::move(notification)), error::success);

    rpc::request last{};
    BOOST_REQUIRE_EQUAL(read(last), error::success);
    BOOST_REQUIRE_EQUAL(std::get<rpc::string_t>(*last.message.id), identity);
    BOOST_REQUIRE_EQUAL(last.message.method, "done");
}

BOOST_AUTO_TEST_SUITE_END()
