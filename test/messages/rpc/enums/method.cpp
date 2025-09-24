/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include "../../../test.hpp"

BOOST_AUTO_TEST_SUITE(rpc_method_tests)

using namespace network::messages::rpc;

BOOST_AUTO_TEST_CASE(rpc_method__to_method__always__expected)
{
    // Default
    BOOST_REQUIRE(to_method("UNDEFINED") == method::undefined);

    // HTTP Verbs
    BOOST_REQUIRE(to_method("GET") == method::get);
    BOOST_REQUIRE(to_method("POST") == method::post);
    BOOST_REQUIRE(to_method("PUT") == method::put);
    BOOST_REQUIRE(to_method("PATCH") == method::patch);
    BOOST_REQUIRE(to_method("DELETE") == method::delete_);
    BOOST_REQUIRE(to_method("HEAD") == method::head);
    BOOST_REQUIRE(to_method("OPTIONS") == method::options);
    BOOST_REQUIRE(to_method("TRACE") == method::trace);
    BOOST_REQUIRE(to_method("CONNECT") == method::connect);
}

BOOST_AUTO_TEST_CASE(rpc_method__from_method__always__expected)
{
    // Default
    BOOST_REQUIRE_EQUAL(from_method(method::undefined), "UNDEFINED");

    // HTTP Verbs
    BOOST_REQUIRE_EQUAL(from_method(method::get), "GET");
    BOOST_REQUIRE_EQUAL(from_method(method::post), "POST");
    BOOST_REQUIRE_EQUAL(from_method(method::put), "PUT");
    BOOST_REQUIRE_EQUAL(from_method(method::patch), "PATCH");
    BOOST_REQUIRE_EQUAL(from_method(method::delete_), "DELETE");
    BOOST_REQUIRE_EQUAL(from_method(method::head), "HEAD");
    BOOST_REQUIRE_EQUAL(from_method(method::options), "OPTIONS");
    BOOST_REQUIRE_EQUAL(from_method(method::trace), "TRACE");
    BOOST_REQUIRE_EQUAL(from_method(method::connect), "CONNECT");
}

BOOST_AUTO_TEST_SUITE_END()
