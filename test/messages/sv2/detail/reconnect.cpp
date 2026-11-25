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

BC_AUTO_TEST_SUITE(sv2_reconnect_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_reconnect__properties__always__expected)
{
    constexpr auto index = registry::index_of<reconnect>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "reconnect");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x04u);
    BC_REQUIRE_EQUAL(registry::channels().at(index), false);
}

BC_AUTO_TEST_CASE(sv2_reconnect__serialize__sample__expected_frame)
{
    const reconnect message{ "text1", 0x0212_u16 };
    BC_REQUIRE_EQUAL(message.size(), 8u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000040800000574657874311202");
}

BC_AUTO_TEST_CASE(sv2_reconnect__deserialize__sample__round_trip)
{
    const auto message = reconnect::deserialize(base16_chunk("0574657874311202"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0000040800000574657874311202");
}

BC_AUTO_TEST_CASE(sv2_reconnect__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!reconnect::deserialize(base16_chunk("05746578743112")));
}

BC_AUTO_TEST_CASE(sv2_reconnect__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!reconnect::deserialize(base16_chunk("057465787431120200")));
}

BC_AUTO_TEST_SUITE_END()
