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
#include "../../../test.hpp"

BC_AUTO_TEST_SUITE(p2p_version_acknowledge_tests)

using namespace network::messages::peer;

BC_AUTO_TEST_CASE(version_acknowledge__properties__always__expected)
{
    BC_REQUIRE_EQUAL(version_acknowledge::command, "verack");
    constexpr auto index = messages::peer::registry::index_of<version_acknowledge>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), version_acknowledge::command);
    BC_REQUIRE_EQUAL(version_acknowledge::version_minimum, level::minimum_protocol);
    BC_REQUIRE_EQUAL(version_acknowledge::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(version_acknowledge__size__always__zero)
{
    BC_REQUIRE_EQUAL(version_acknowledge::size(level::canonical), zero);
}

BC_AUTO_TEST_CASE(version_acknowledge__deserialize1__empty__expected)
{
    const system::data_chunk data{};
    BC_REQUIRE(version_acknowledge::deserialize(level::minimum_protocol, data));
}

BC_AUTO_TEST_CASE(version_acknowledge__deserialize1__insufficient_version__nullptr)
{
    const system::data_chunk data{};
    BC_REQUIRE(!version_acknowledge::deserialize(level::minimum_protocol - 1u, data));
}

BC_AUTO_TEST_CASE(version_acknowledge__deserialize1__excessive_version__nullptr)
{
    const system::data_chunk data{};
    BC_REQUIRE(!version_acknowledge::deserialize(level::maximum_protocol + 1u, data));
}

BC_AUTO_TEST_CASE(version_acknowledge__deserialize2__insufficient_version__source_false)
{
    const system::data_chunk data{};
    system::read::bytes::copy source(data);
    version_acknowledge::deserialize(level::minimum_protocol - 1u, source);
    BC_REQUIRE(!source);
}

BC_AUTO_TEST_CASE(version_acknowledge__serialize1__default__empty)
{
    system::data_chunk data{};
    BC_REQUIRE(version_acknowledge{}.serialize(level::minimum_protocol, data));
    BC_REQUIRE(data.empty());
}

BC_AUTO_TEST_SUITE_END()
