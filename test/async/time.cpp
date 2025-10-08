/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

// Fixed time_t for 2021-07-01 00:00:00 UTC (Thu).
constexpr time_t test_time{ 1625097600 };

// Near Year 2038 boundary: 2038-01-19 03:14:07 UTC.
constexpr time_t year_2038_time{ 2147483647 };

// zulu_time

BOOST_AUTO_TEST_CASE(time__zulu_time__always__non_default)
{
    BOOST_REQUIRE_NE(zulu_time(), time_t{});
}

// unix_time

BOOST_AUTO_TEST_CASE(time__unix_time__always__non_default)
{
    BOOST_REQUIRE_NE(unix_time(), uint32_t{});
}

// format_local_time

BOOST_AUTO_TEST_CASE(time__format_local_time__always__non_empty)
{
    // This only works in one time zone.
    ////BOOST_REQUIRE_EQUAL(format_local_time(0x12345678_u32), "1979-09-05T15:51:36");
    BOOST_REQUIRE(!format_local_time(0x12345678_u32).empty());
}

BOOST_AUTO_TEST_CASE(time__format_local_time__year_2038_boundary__correct_format)
{
    const auto result = format_local_time(year_2038_time);
    BOOST_REQUIRE(!result.empty());
    BOOST_REQUIRE(result.size() >= 19);
    BOOST_REQUIRE_EQUAL(result.substr(0, 4), "2038");
}

// format_zulu_time

BOOST_AUTO_TEST_CASE(time__format_zulu_time__always__expected)
{
    BOOST_REQUIRE_EQUAL(format_zulu_time(0x12345678_u32), "1979-09-05T22:51:36Z");
}

BOOST_AUTO_TEST_CASE(time__format_zulu_time__valid_time__rfc3339_format)
{
    BOOST_REQUIRE_EQUAL(format_zulu_time(test_time), "2021-07-01T00:00:00Z");
}

BOOST_AUTO_TEST_CASE(time__format_zulu_time__year_2038_boundary__rfc3339_format)
{
    BOOST_REQUIRE_EQUAL(format_zulu_time(year_2038_time), "2038-01-19T03:14:07Z");
}

// format_http_time

BOOST_AUTO_TEST_CASE(time__format_http_time__valid_time__rfc7231_format)
{
    BOOST_REQUIRE_EQUAL(format_http_time(test_time), "Thu, 01 Jul 2021 00:00:00 GMT");
}

BOOST_AUTO_TEST_CASE(time__format_http_time__year_2038_boundary__rfc7231_format)
{
    BOOST_REQUIRE_EQUAL(format_http_time(year_2038_time), "Tue, 19 Jan 2038 03:14:07 GMT");
}

BOOST_AUTO_TEST_SUITE_END()
