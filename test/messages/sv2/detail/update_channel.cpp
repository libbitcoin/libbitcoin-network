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

BC_AUTO_TEST_SUITE(sv2_update_channel_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_update_channel__properties__always__expected)
{
    constexpr auto index = registry::index_of<update_channel>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "update_channel");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x16u);
    BC_REQUIRE_EQUAL(registry::channels().at(index), true);
}

BC_AUTO_TEST_CASE(sv2_update_channel__serialize__sample__expected_frame)
{
    const update_channel message{ 0x02020301_u32, 3.0f, base16_array("303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f") };
    BC_REQUIRE_EQUAL(message.size(), 40u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0080162800000103020200004040303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f");
}

BC_AUTO_TEST_CASE(sv2_update_channel__deserialize__sample__round_trip)
{
    const auto message = update_channel::deserialize(base16_chunk("0103020200004040303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0080162800000103020200004040303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f");
}

BC_AUTO_TEST_CASE(sv2_update_channel__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!update_channel::deserialize(base16_chunk("0103020200004040303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e")));
}

BC_AUTO_TEST_CASE(sv2_update_channel__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!update_channel::deserialize(base16_chunk("0103020200004040303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f00")));
}

BC_AUTO_TEST_SUITE_END()
