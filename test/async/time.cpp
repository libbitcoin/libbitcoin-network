/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(time_tests)

BOOST_AUTO_TEST_CASE(time__zulu_time__always__non_default)
{
    BOOST_REQUIRE_NE(zulu_time(), time_t{});
}

BOOST_AUTO_TEST_CASE(time__unix_time__always__non_default)
{
    BOOST_REQUIRE_NE(unix_time(), uint32_t{});
}

BOOST_AUTO_TEST_CASE(time__format_local_time__always__non_empty)
{
    // This only works in one time zone.
    ////BOOST_REQUIRE_EQUAL(format_local_time(0x12345678_u32), "1979-09-05T15:51:36");
    BOOST_REQUIRE(!format_local_time(0x12345678_u32).empty());
}

BOOST_AUTO_TEST_CASE(time__format_zulu_time__always__expected)
{
    BOOST_REQUIRE_EQUAL(format_zulu_time(0x12345678_u32), "1979-09-05T22:51:36Z");
}

BOOST_AUTO_TEST_SUITE_END()
