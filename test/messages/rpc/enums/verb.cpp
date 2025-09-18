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

BOOST_AUTO_TEST_SUITE(rpc_verb_tests)

using namespace network::messages::rpc;

BOOST_AUTO_TEST_CASE(rpc_verb__to_verb__always__expected)
{
    // Default
    BOOST_REQUIRE(to_verb("UNDEFINED") == verb::undefined);

    // HTTP Verbs
    BOOST_REQUIRE(to_verb("GET") == verb::get);
    BOOST_REQUIRE(to_verb("POST") == verb::post);
    BOOST_REQUIRE(to_verb("PUT") == verb::put);
    BOOST_REQUIRE(to_verb("PATCH") == verb::patch);
    BOOST_REQUIRE(to_verb("DELETE") == verb::delete_);
    BOOST_REQUIRE(to_verb("HEAD") == verb::head);
    BOOST_REQUIRE(to_verb("OPTIONS") == verb::options);
    BOOST_REQUIRE(to_verb("TRACE") == verb::trace);
    BOOST_REQUIRE(to_verb("CONNECT") == verb::connect);
}

BOOST_AUTO_TEST_CASE(rpc_verb__from_verb__always__expected)
{
    // Default
    BOOST_REQUIRE_EQUAL(from_verb(verb::undefined), "UNDEFINED");

    // HTTP Verbs
    BOOST_REQUIRE_EQUAL(from_verb(verb::get), "GET");
    BOOST_REQUIRE_EQUAL(from_verb(verb::post), "POST");
    BOOST_REQUIRE_EQUAL(from_verb(verb::put), "PUT");
    BOOST_REQUIRE_EQUAL(from_verb(verb::patch), "PATCH");
    BOOST_REQUIRE_EQUAL(from_verb(verb::delete_), "DELETE");
    BOOST_REQUIRE_EQUAL(from_verb(verb::head), "HEAD");
    BOOST_REQUIRE_EQUAL(from_verb(verb::options), "OPTIONS");
    BOOST_REQUIRE_EQUAL(from_verb(verb::trace), "TRACE");
    BOOST_REQUIRE_EQUAL(from_verb(verb::connect), "CONNECT");
}

BOOST_AUTO_TEST_SUITE_END()
