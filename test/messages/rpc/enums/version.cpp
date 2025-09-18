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

BOOST_AUTO_TEST_SUITE(rpc_version_tests)

using namespace network::messages::rpc;

BOOST_AUTO_TEST_CASE(rpc_version__to_version__always__expected)
{
    // Default
    BOOST_REQUIRE(to_version("UNDEFINED/0.0") == version::undefined);

    // HTTP Versions
    BOOST_REQUIRE(to_version("HTTP/0.9") == version::http_0_9);
    BOOST_REQUIRE(to_version("HTTP/1.0") == version::http_1_0);
    BOOST_REQUIRE(to_version("HTTP/1.1") == version::http_1_1);
}

BOOST_AUTO_TEST_CASE(rpc_version__from_version__always__expected)
{
    // Default
    BOOST_REQUIRE_EQUAL(from_version(version::undefined), "UNDEFINED/0.0");

    // HTTP Versions
    BOOST_REQUIRE_EQUAL(from_version(version::http_0_9), "HTTP/0.9");
    BOOST_REQUIRE_EQUAL(from_version(version::http_1_0), "HTTP/1.0");
    BOOST_REQUIRE_EQUAL(from_version(version::http_1_1), "HTTP/1.1");
}

BOOST_AUTO_TEST_SUITE_END()
