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

BC_AUTO_TEST_SUITE(sv2_open_standard_mining_channel_success_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_open_standard_mining_channel_success__properties__always__expected)
{
    constexpr auto index = registry::index_of<open_standard_mining_channel_success>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "open_standard_mining_channel_success");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x11u);
    BC_REQUIRE_EQUAL(registry::channels().at(index), false);
}

BC_AUTO_TEST_CASE(sv2_open_standard_mining_channel_success__serialize__sample__expected_frame)
{
    const open_standard_mining_channel_success message{ 0x02020301_u32, 0x03020302_u32, base16_array("303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f"), base16_chunk("a4b4c4"), 0x06020305_u32 };
    BC_REQUIRE_EQUAL(message.size(), 48u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000113000000103020202030203303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f03a4b4c405030206");
}

BC_AUTO_TEST_CASE(sv2_open_standard_mining_channel_success__deserialize__sample__round_trip)
{
    const auto message = open_standard_mining_channel_success::deserialize(base16_chunk("0103020202030203303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f03a4b4c405030206"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000113000000103020202030203303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f03a4b4c405030206");
}

BC_AUTO_TEST_CASE(sv2_open_standard_mining_channel_success__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!open_standard_mining_channel_success::deserialize(base16_chunk("0103020202030203303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f03a4b4c4050302")));
}

BC_AUTO_TEST_CASE(sv2_open_standard_mining_channel_success__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!open_standard_mining_channel_success::deserialize(base16_chunk("0103020202030203303132333435363738393a3b3c3d3e3f404142434445464748494a4b4c4d4e4f03a4b4c40503020600")));
}

BC_AUTO_TEST_SUITE_END()
