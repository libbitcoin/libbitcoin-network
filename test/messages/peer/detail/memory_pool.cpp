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

BOOST_AUTO_TEST_SUITE(p2p_memory_pool_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(memory_pool__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(memory_pool::command, "mempool");
    constexpr auto index = messages::peer::registry::index_of<memory_pool>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), memory_pool::command);
    BOOST_REQUIRE_EQUAL(memory_pool::version_minimum, level::bip35);
    BOOST_REQUIRE_EQUAL(memory_pool::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(memory_pool__size__always_zero)
{
    BOOST_REQUIRE_EQUAL(memory_pool::size(level::canonical), zero);
}

BOOST_AUTO_TEST_CASE(memory_pool__deserialize1__empty__expected)
{
    const system::data_chunk data{};
    BOOST_REQUIRE(memory_pool::deserialize(level::bip35, data));
}

BOOST_AUTO_TEST_CASE(memory_pool__deserialize1__insufficient_version__nullptr)
{
    const system::data_chunk data{};
    BOOST_REQUIRE(!memory_pool::deserialize(level::bip35 - 1u, data));
}

BOOST_AUTO_TEST_CASE(memory_pool__deserialize1__excessive_version__nullptr)
{
    const system::data_chunk data{};
    BOOST_REQUIRE(!memory_pool::deserialize(level::maximum_protocol + 1u, data));
}

BOOST_AUTO_TEST_CASE(memory_pool__deserialize2__insufficient_version__source_false)
{
    const system::data_chunk data{};
    system::read::bytes::copy source(data);
    memory_pool::deserialize(level::bip35 - 1u, source);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_CASE(memory_pool__serialize1__default__empty)
{
    system::data_chunk data{};
    BOOST_REQUIRE(memory_pool{}.serialize(level::bip35, data));
    BOOST_REQUIRE(data.empty());
}

BOOST_AUTO_TEST_SUITE_END()
