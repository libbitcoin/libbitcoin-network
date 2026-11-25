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

BC_AUTO_TEST_SUITE(sv2_open_extended_mining_channel_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_open_extended_mining_channel__properties__always__expected)
{
    constexpr auto index = registry::index_of<open_extended_mining_channel>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "open_extended_mining_channel");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x13u);
    BC_REQUIRE_EQUAL(registry::channels().at(index), false);
}

BC_AUTO_TEST_CASE(sv2_open_extended_mining_channel__serialize__sample__expected_frame)
{
    const open_extended_mining_channel message{ 0x02020301_u32, "text2", 4.5f, base16_array("404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f"), 0x0515_u16 };
    BC_REQUIRE_EQUAL(message.size(), 48u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000133000000103020205746578743200009040404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f1505");
}

BC_AUTO_TEST_CASE(sv2_open_extended_mining_channel__deserialize__sample__round_trip)
{
    const auto message = open_extended_mining_channel::deserialize(base16_chunk("0103020205746578743200009040404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f1505"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000133000000103020205746578743200009040404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f1505");
}

BC_AUTO_TEST_CASE(sv2_open_extended_mining_channel__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!open_extended_mining_channel::deserialize(base16_chunk("0103020205746578743200009040404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f15")));
}

BC_AUTO_TEST_CASE(sv2_open_extended_mining_channel__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!open_extended_mining_channel::deserialize(base16_chunk("0103020205746578743200009040404142434445464748494a4b4c4d4e4f505152535455565758595a5b5c5d5e5f150500")));
}

BC_AUTO_TEST_SUITE_END()
