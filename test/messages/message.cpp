/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(message_tests)

using namespace system;
using namespace bc::network::messages;

constexpr auto empty_checksum = 0xe2e0f65d_u32;
constexpr auto empty_hash = sha256::double_hash(sha256::ablocks_t<0>{});
static_assert(from_little_endian<uint32_t>(empty_hash) == empty_checksum);

BOOST_AUTO_TEST_CASE(message__network_checksum__empty_hash__empty_checksum)
{
    BOOST_REQUIRE_EQUAL(network_checksum(empty_hash), empty_checksum);
}

BOOST_AUTO_TEST_SUITE_END()
