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
#include "test.hpp"

BOOST_AUTO_TEST_SUITE(client_tests)

BOOST_AUTO_TEST_CASE(client__tcp_server__defaults__expected)
{
    const tcp_server instance{};

    // tcp_server
    BOOST_REQUIRE(!instance.secure);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.timeout_seconds, 60u);
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_CASE(client__http_server__defaults__expected)
{
    const http_server instance{};

    // tcp_server
    BOOST_REQUIRE(!instance.secure);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.timeout_seconds, 60u);
    BOOST_REQUIRE(!instance.enabled());

    // http_server
    BOOST_REQUIRE_EQUAL(instance.server, "libbitcoin/4.0");
    BOOST_REQUIRE(instance.hosts.empty());
    BOOST_REQUIRE(instance.host_names().empty());
}

BOOST_AUTO_TEST_CASE(client__html_server__defaults__expected)
{
    const html_server instance{};

    // tcp_server
    BOOST_REQUIRE(!instance.secure);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE_EQUAL(instance.timeout_seconds, 60u);
    BOOST_REQUIRE(!instance.enabled());

    // http_server
    BOOST_REQUIRE_EQUAL(instance.server, "libbitcoin/4.0");
    BOOST_REQUIRE(instance.hosts.empty());
    BOOST_REQUIRE(instance.host_names().empty());

    // html_server
    BOOST_REQUIRE(instance.path.empty());
    BOOST_REQUIRE_EQUAL(instance.default_, "index.html");
    BOOST_REQUIRE(instance.origins.empty());
    BOOST_REQUIRE(instance.origin_names().empty());
}

BOOST_AUTO_TEST_SUITE_END()
