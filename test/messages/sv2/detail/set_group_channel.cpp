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

BC_AUTO_TEST_SUITE(sv2_set_group_channel_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_set_group_channel__properties__always__expected)
{
    constexpr auto index = registry::index_of<set_group_channel>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "set_group_channel");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x25u);
    BC_REQUIRE_EQUAL(registry::channels().at(index), false);
}

BC_AUTO_TEST_CASE(sv2_set_group_channel__serialize__sample__expected_frame)
{
    const set_group_channel message{ 0x02020301_u32, std::vector<uint32_t>{ 0x00000007_u32, 0x00000009_u32 } };
    BC_REQUIRE_EQUAL(message.size(), 14u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000250e00000103020202000700000009000000");
}

BC_AUTO_TEST_CASE(sv2_set_group_channel__deserialize__sample__round_trip)
{
    const auto message = set_group_channel::deserialize(base16_chunk("0103020202000700000009000000"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000250e00000103020202000700000009000000");
}

BC_AUTO_TEST_CASE(sv2_set_group_channel__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!set_group_channel::deserialize(base16_chunk("01030202020007000000090000")));
}

BC_AUTO_TEST_CASE(sv2_set_group_channel__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!set_group_channel::deserialize(base16_chunk("010302020200070000000900000000")));
}

BC_AUTO_TEST_SUITE_END()
