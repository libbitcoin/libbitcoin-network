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

BC_AUTO_TEST_SUITE(p2p_send_headers_tests)

using namespace network::messages::peer;

BC_AUTO_TEST_CASE(send_headers__properties__always__expected)
{
    BC_REQUIRE_EQUAL(send_headers::command, "sendheaders");
    constexpr auto index = messages::peer::registry::index_of<send_headers>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), send_headers::command);
    BC_REQUIRE_EQUAL(send_headers::version_minimum, level::bip130);
    BC_REQUIRE_EQUAL(send_headers::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(send_headers__size__always__zero)
{
    BC_REQUIRE_EQUAL(send_headers::size(level::canonical), zero);
}

BC_AUTO_TEST_CASE(send_headers__deserialize1__empty__expected)
{
    const system::data_chunk data{};
    BC_REQUIRE(send_headers::deserialize(level::bip130, data));
}

BC_AUTO_TEST_CASE(send_headers__deserialize1__insufficient_version__nullptr)
{
    const system::data_chunk data{};
    BC_REQUIRE(!send_headers::deserialize(level::bip130 - 1u, data));
}

BC_AUTO_TEST_CASE(send_headers__deserialize1__excessive_version__nullptr)
{
    const system::data_chunk data{};
    BC_REQUIRE(!send_headers::deserialize(level::maximum_protocol + 1u, data));
}

BC_AUTO_TEST_CASE(send_headers__deserialize2__insufficient_version__source_false)
{
    const system::data_chunk data{};
    system::read::bytes::copy source(data);
    send_headers::deserialize(level::bip130 - 1u, source);
    BC_REQUIRE(!source);
}

BC_AUTO_TEST_CASE(send_headers__serialize1__default__empty)
{
    system::data_chunk data{};
    BC_REQUIRE(send_headers{}.serialize(level::bip130, data));
    BC_REQUIRE(data.empty());
}

BC_AUTO_TEST_SUITE_END()
