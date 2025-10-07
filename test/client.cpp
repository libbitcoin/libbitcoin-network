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

BOOST_AUTO_TEST_CASE(client__admin__defaults__expected)
{
    const admin instance{};
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE(instance.path.empty());
    BOOST_REQUIRE(instance.hosts.empty());
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_CASE(client__explore__defaults__expected)
{
    const explore instance{};
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE(instance.path.empty());
    BOOST_REQUIRE(instance.hosts.empty());
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_CASE(client__rest__defaults__expected)
{
    const rest instance{};
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE(instance.hosts.empty());
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_CASE(client__websocket__defaults__expected)
{
    const websocket instance{};
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE(instance.hosts.empty());
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_CASE(client__bitcoind__defaults__expected)
{
    const bitcoind instance{};
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE(instance.hosts.empty());
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_CASE(client__electrum__defaults__expected)
{
    const electrum instance{};
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_CASE(client__stratum_v1__defaults__expected)
{
    const stratum_v1 instance{};
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_CASE(client__stratum_v2__defaults__expected)
{
    const stratum_v2 instance{};
    BOOST_REQUIRE_EQUAL(instance.connections, 0u);
    BOOST_REQUIRE(instance.binds.empty());
    BOOST_REQUIRE(!instance.enabled());
}

BOOST_AUTO_TEST_SUITE_END()
