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

BC_AUTO_TEST_SUITE(sv2_setup_connection_success_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_setup_connection_success__properties__always__expected)
{
    constexpr auto index = registry::index_of<setup_connection_success>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "setup_connection_success");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x01u);
    BC_REQUIRE_EQUAL(registry::channels().at(index), false);
}

BC_AUTO_TEST_CASE(sv2_setup_connection_success__serialize__sample__expected_frame)
{
    const setup_connection_success message{ 0x0111_u16, 0x03020302_u32 };
    BC_REQUIRE_EQUAL(message.size(), 6u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "000001060000110102030203");
}

BC_AUTO_TEST_CASE(sv2_setup_connection_success__deserialize__sample__round_trip)
{
    const auto message = setup_connection_success::deserialize(base16_chunk("110102030203"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "000001060000110102030203");
}

BC_AUTO_TEST_CASE(sv2_setup_connection_success__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!setup_connection_success::deserialize(base16_chunk("1101020302")));
}

BC_AUTO_TEST_CASE(sv2_setup_connection_success__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!setup_connection_success::deserialize(base16_chunk("11010203020300")));
}

BC_AUTO_TEST_SUITE_END()
