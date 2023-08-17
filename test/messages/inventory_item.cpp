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

BOOST_AUTO_TEST_SUITE(inventory_item_tests)

using namespace bc::network::messages;

// to_number
// to_type
// to_string
// is_block_type
// is_transaction_type
// is_witnessable_type
// to_witness

BOOST_AUTO_TEST_CASE(inventory_item__size__always__expected)
{
    constexpr auto expected = system::hash_size
        + sizeof(uint32_t);

    BOOST_REQUIRE_EQUAL(inventory_item::size(level::canonical), expected);
}

BOOST_AUTO_TEST_SUITE_END()
