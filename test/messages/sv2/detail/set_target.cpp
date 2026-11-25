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

BC_AUTO_TEST_SUITE(sv2_set_target_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_set_target__properties__always__expected)
{
    constexpr auto index = registry::index_of<set_target>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "set_target");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x21u);
    BC_REQUIRE_EQUAL(registry::channels().at(index), true);
}

BC_AUTO_TEST_CASE(sv2_set_target__serialize__sample__expected_frame)
{
    const set_target message{ 0x02020301_u32, base16_array("202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f") };
    BC_REQUIRE_EQUAL(message.size(), 36u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "00802124000001030202202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
}

BC_AUTO_TEST_CASE(sv2_set_target__deserialize__sample__round_trip)
{
    const auto message = set_target::deserialize(base16_chunk("01030202202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "00802124000001030202202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
}

BC_AUTO_TEST_CASE(sv2_set_target__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!set_target::deserialize(base16_chunk("01030202202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e")));
}

BC_AUTO_TEST_CASE(sv2_set_target__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!set_target::deserialize(base16_chunk("01030202202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f00")));
}

BC_AUTO_TEST_SUITE_END()
