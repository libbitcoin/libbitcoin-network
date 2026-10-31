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

BC_AUTO_TEST_SUITE(zmtp_role_tests)

BC_AUTO_TEST_CASE(zmtp_role__socket_type__each_role__expected)
{
    BC_REQUIRE_EQUAL(socket_type(zmtp_role::publisher), "PUB");
    BC_REQUIRE_EQUAL(socket_type(zmtp_role::puller), "PULL");
    BC_REQUIRE_EQUAL(socket_type(zmtp_role::replier), "REP");
    BC_REQUIRE_EQUAL(socket_type(zmtp_role::router), "ROUTER");
    BC_REQUIRE(socket_type(zmtp_role::undefined).empty());
}

BC_AUTO_TEST_CASE(zmtp_role__compatible__peer_socket_types__expected)
{
    BC_REQUIRE(compatible(zmtp_role::publisher, "SUB"));
    BC_REQUIRE(compatible(zmtp_role::publisher, "XSUB"));
    BC_REQUIRE(!compatible(zmtp_role::publisher, "PUSH"));

    BC_REQUIRE(compatible(zmtp_role::puller, "PUSH"));
    BC_REQUIRE(!compatible(zmtp_role::puller, "SUB"));

    BC_REQUIRE(compatible(zmtp_role::replier, "REQ"));
    BC_REQUIRE(compatible(zmtp_role::replier, "DEALER"));
    BC_REQUIRE(!compatible(zmtp_role::replier, "ROUTER"));

    BC_REQUIRE(compatible(zmtp_role::router, "REQ"));
    BC_REQUIRE(compatible(zmtp_role::router, "DEALER"));
    BC_REQUIRE(compatible(zmtp_role::router, "ROUTER"));

    BC_REQUIRE(!compatible(zmtp_role::undefined, "SUB"));
}

BC_AUTO_TEST_SUITE_END()

// PUSH peer sends three [push][one][two] messages.
// ----------------------------------------------------------------------------

BC_FIXTURE_TEST_SUITE(zmtp_role_puller_tests, role_puller_fixture)

BC_AUTO_TEST_CASE(zmtp_role__puller__push_peer__expected)
{
    if (!enabled)
        return;

    rpc::request first{};
    BC_REQUIRE_EQUAL(read(first), error::success);
    BC_REQUIRE_EQUAL(first.message.method, "push");
    BC_REQUIRE_EQUAL(params_of(first), 2u);
    BC_REQUIRE_EQUAL(param_of(first, 0), chunk("one"));
    BC_REQUIRE_EQUAL(param_of(first, 1), chunk("two"));

    rpc::request second{};
    BC_REQUIRE_EQUAL(read(second), error::success);
    BC_REQUIRE_EQUAL(second.message.method, "push");
    BC_REQUIRE_EQUAL(params_of(second), 2u);
    BC_REQUIRE_EQUAL(param_of(second, 0), chunk("one"));
    BC_REQUIRE_EQUAL(param_of(second, 1), chunk("two"));

    rpc::request third{};
    BC_REQUIRE_EQUAL(read(third), error::success);
    BC_REQUIRE_EQUAL(third.message.method, "push");
    BC_REQUIRE_EQUAL(params_of(third), 2u);
    BC_REQUIRE_EQUAL(param_of(third, 0), chunk("one"));
    BC_REQUIRE_EQUAL(param_of(third, 1), chunk("two"));
}

BC_AUTO_TEST_SUITE_END()

// REQ peer sends [echo][text] twice (answered with [text]) then [fail]
// (answered with an error, code and message).
// ----------------------------------------------------------------------------

BC_FIXTURE_TEST_SUITE(zmtp_role_replier_tests, role_replier_fixture)

BC_AUTO_TEST_CASE(zmtp_role__replier__req_peer__expected)
{
    if (!enabled)
        return;

    rpc::request first{};
    BC_REQUIRE_EQUAL(read(first), error::success);
    BC_REQUIRE_EQUAL(first.message.method, "echo");
    BC_REQUIRE_EQUAL(params_of(first), 1u);

    rpc::response first_reply{};
    first_reply.message.result = rpc::value_t{ rpc::any_t{ system::to_shared(param_of(first, 0)) } };
    BC_REQUIRE_EQUAL(respond(std::move(first_reply)), error::success);

    rpc::request second{};
    BC_REQUIRE_EQUAL(read(second), error::success);
    BC_REQUIRE_EQUAL(second.message.method, "echo");
    BC_REQUIRE_EQUAL(params_of(second), 1u);

    rpc::response second_reply{};
    second_reply.message.result = rpc::value_t{ rpc::any_t{ system::to_shared(param_of(second, 0)) } };
    BC_REQUIRE_EQUAL(respond(std::move(second_reply)), error::success);

    rpc::request third{};
    BC_REQUIRE_EQUAL(read(third), error::success);
    BC_REQUIRE_EQUAL(third.message.method, "fail");

    rpc::response error_reply{};
    error_reply.message.error = rpc::result_t{ .code = -1, .message = "nope" };
    BC_REQUIRE_EQUAL(respond(std::move(error_reply)), error::success);
}

BC_AUTO_TEST_SUITE_END()

// DEALER peer (routing id "peer1", REQ envelope) sends [][echo][text],
// answered with [][text] and then notified with [][topic][note], then sends
// [][done].
// ----------------------------------------------------------------------------

BC_FIXTURE_TEST_SUITE(zmtp_role_router_tests, role_router_fixture)

BC_AUTO_TEST_CASE(zmtp_role__router__dealer_peer__expected)
{
    if (!enabled)
        return;

    rpc::request request{};
    BC_REQUIRE_EQUAL(read(request), error::success);
    BC_REQUIRE(request.message.id);
    BC_REQUIRE_EQUAL(std::get<rpc::string_t>(*request.message.id), "peer1");
    BC_REQUIRE_EQUAL(request.message.method, "echo");
    BC_REQUIRE_EQUAL(params_of(request), 1u);
    const auto identity = std::get<rpc::string_t>(*request.message.id);

    rpc::response response{};
    response.message.id = identity;
    response.message.result = rpc::value_t{ rpc::any_t{ system::to_shared(param_of(request, 0)) } };
    BC_REQUIRE_EQUAL(respond(std::move(response)), error::success);

    rpc::request notification{};
    notification.message.id = identity;
    notification.message.method = "topic";
    notification.message.params = rpc::array_t{ rpc::any_t{ system::to_shared(chunk("note")) } };
    BC_REQUIRE_EQUAL(notify(std::move(notification)), error::success);

    rpc::request last{};
    BC_REQUIRE_EQUAL(read(last), error::success);
    BC_REQUIRE_EQUAL(std::get<rpc::string_t>(*last.message.id), identity);
    BC_REQUIRE_EQUAL(last.message.method, "done");
}

BC_AUTO_TEST_SUITE_END()
