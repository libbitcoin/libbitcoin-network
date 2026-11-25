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

BC_AUTO_TEST_SUITE(sv2_setup_connection_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_setup_connection__properties__always__expected)
{
    constexpr auto index = registry::index_of<setup_connection>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "setup_connection");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x00u);
    BC_REQUIRE_EQUAL(registry::channels().at(index), false);
}

BC_AUTO_TEST_CASE(sv2_setup_connection__serialize__sample__expected_frame)
{
    const setup_connection message{ 0x01_u8, 0x0212_u16, 0x0313_u16, 0x05020304_u32, "text5", 0x0616_u16, "text7", "text8", "text9", "text10" };
    BC_REQUIRE_EQUAL(message.size(), 42u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000002a0000011202130304030205057465787435160605746578743705746578743805746578743906746578743130");
}

BC_AUTO_TEST_CASE(sv2_setup_connection__deserialize__sample__round_trip)
{
    const auto message = setup_connection::deserialize(base16_chunk("011202130304030205057465787435160605746578743705746578743805746578743906746578743130"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000002a0000011202130304030205057465787435160605746578743705746578743805746578743906746578743130");
}

BC_AUTO_TEST_CASE(sv2_setup_connection__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!setup_connection::deserialize(base16_chunk("0112021303040302050574657874351606057465787437057465787438057465787439067465787431")));
}

BC_AUTO_TEST_CASE(sv2_setup_connection__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!setup_connection::deserialize(base16_chunk("01120213030403020505746578743516060574657874370574657874380574657874390674657874313000")));
}

BC_AUTO_TEST_SUITE_END()
