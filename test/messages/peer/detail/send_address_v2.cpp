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

BC_AUTO_TEST_SUITE(p2p_send_address_v2_tests)

using namespace network::messages::peer;

BC_AUTO_TEST_CASE(send_address_v2__properties__always__expected)
{
    BC_REQUIRE_EQUAL(send_address_v2::command, "sendaddrv2");
    constexpr auto index = messages::peer::registry::index_of<send_address_v2>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), send_address_v2::command);
    BC_REQUIRE_EQUAL(send_address_v2::version_minimum, level::bip155);
    BC_REQUIRE_EQUAL(send_address_v2::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(send_address_v2__size__always__expected)
{
    BC_REQUIRE_EQUAL(send_address_v2::size(level::canonical), zero);
}

BC_AUTO_TEST_CASE(send_address_v2__deserialize1__empty__expected)
{
    const system::data_chunk data{};
    BC_REQUIRE(send_address_v2::deserialize(level::bip155, data));
}

BC_AUTO_TEST_CASE(send_address_v2__deserialize1__insufficient_version__nullptr)
{
    const system::data_chunk data{};
    BC_REQUIRE(!send_address_v2::deserialize(level::bip155 - 1u, data));
}

BC_AUTO_TEST_CASE(send_address_v2__deserialize1__excessive_version__nullptr)
{
    const system::data_chunk data{};
    BC_REQUIRE(!send_address_v2::deserialize(level::maximum_protocol + 1u, data));
}

BC_AUTO_TEST_CASE(send_address_v2__deserialize2__insufficient_version__source_false)
{
    const system::data_chunk data{};
    system::read::bytes::copy source(data);
    send_address_v2::deserialize(level::bip155 - 1u, source);
    BC_REQUIRE(!source);
}

BC_AUTO_TEST_CASE(send_address_v2__serialize1__default__empty)
{
    system::data_chunk data{};
    BC_REQUIRE(send_address_v2{}.serialize(level::bip155, data));
    BC_REQUIRE(data.empty());
}

BC_AUTO_TEST_SUITE_END()
