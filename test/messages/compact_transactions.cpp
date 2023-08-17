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

BOOST_AUTO_TEST_SUITE(compact_transactions_tests)

using namespace bc::network::messages;

BOOST_AUTO_TEST_CASE(ccompact_transactions__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(compact_transactions::command, "blocktxn");
    BOOST_REQUIRE(compact_transactions::id == identifier::compact_transactions);
    BOOST_REQUIRE_EQUAL(compact_transactions::version_minimum, level::bip152);
    BOOST_REQUIRE_EQUAL(compact_transactions::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(compact_transactions__size__default__expected)
{
    constexpr auto expected = system::hash_size
        + variable_size(zero);

    BOOST_REQUIRE_EQUAL(compact_transactions{}.size(level::canonical, true), expected);
    BOOST_REQUIRE_EQUAL(compact_transactions{}.size(level::canonical, false), expected);
}

BOOST_AUTO_TEST_SUITE_END()
