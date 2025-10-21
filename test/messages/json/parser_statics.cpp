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
#include "../../test.hpp"

////#include <limits>
////constexpr double infinity = std::numeric_limits<double>::infinity();

BOOST_AUTO_TEST_SUITE(parser_tests)

using namespace network::json;

struct accessor
  : parser<true, json::version::any>
{
    static bool to_number(double& out, view_t token) NOEXCEPT
    {
        return parser<true, json::version::any>::to_number(out, token);
    }
};

BOOST_AUTO_TEST_CASE(parse_json__to_number__zero__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "0"));
    BOOST_CHECK_EQUAL(result, 0.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__negative_zero__success_negative_preserved)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "-0"));
    BOOST_CHECK(std::signbit(result));
    BOOST_CHECK_EQUAL(result, -0.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__positive_integer__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "123"));
    BOOST_CHECK_EQUAL(result, 123.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__negative_integer__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "-123"));
    BOOST_CHECK_EQUAL(result, -123.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__zero_decimal__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "0.0"));
    BOOST_CHECK_EQUAL(result, 0.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__positive_decimal__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "0.123"));
    BOOST_CHECK_CLOSE(result, 0.123, 1e-9);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__negative_decimal__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "-1.234"));
    BOOST_CHECK_CLOSE(result, -1.234, 1e-9);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__integer_decimal__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "1.0"));
    BOOST_CHECK_EQUAL(result, 1.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__positive_exponent__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "1e3"));
    BOOST_CHECK_EQUAL(result, 1000.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__negative_exponent__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "1E-3"));
    BOOST_CHECK_CLOSE(result, 0.001, 1e-9);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__negative_decimal_positive_exponent__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "-1.23e+4"));
    BOOST_CHECK_CLOSE(result, -12300.0, 1e-9);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__decimal_negative_exponent__success)
{
    double result{};
    BOOST_CHECK(accessor::to_number(result, "123.456e-7"));
    BOOST_CHECK_CLOSE(result, 0.0000123456, 1e-9);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__max_double__success)
{
    double result{};

    // Approximate maximum double value.
    const std::string max_str = "1.7976931348623157e308";
    BOOST_CHECK(accessor::to_number(result, max_str));
    BOOST_CHECK_CLOSE(result, std::numeric_limits<double>::max(), 1e-10);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__min_normal_double__success)
{
    double result{};

    // Approximate minimum positive normal double.
    std::string min_normal_str = "2.2250738585072014e-308";
    BOOST_CHECK(accessor::to_number(result, min_normal_str));
    BOOST_CHECK_CLOSE(result, std::numeric_limits<double>::min(), 1e-10);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__large_representable_integer__success)
{
    double result{};

    // 2^53 - 1, exact in double.
    // Test a large but representable integer (within double precision).
    const std::string large_int = "9007199254740991";
    BOOST_CHECK(accessor::to_number(result, large_int));
    BOOST_CHECK_EQUAL(result, 9007199254740991.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__empty_string__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, ""));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__leading_plus__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "+1"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__leading_zero_integer__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "00"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__leading_zero_nonzero__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "0123"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__trailing_decimal_no_digits__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1."));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__leading_decimal__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, ".1"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__exponent_no_digits__fails)
{
    // Initial value to check it remains unmodified on failure.
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1e"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__exponent_plus_no_digits__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1e+"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__exponent_minus_no_digits__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1e-"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__multiple_decimals__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1.2.3"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__trailing_invalid_char__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1e2a"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__invalid_char_in_integer__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1a"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__infinity__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "Infinity"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__nan__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "NaN"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__whitespace__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1 2"));
    ////BOOST_CHECK_EQUAL(result, 42.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__overflow_positive__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1e309"));
    ////BOOST_CHECK_EQUAL(result, infinity);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__overflow_negative__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "-1e309"));
    ////BOOST_CHECK_EQUAL(result, -infinity);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__underflow_positive__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "1e-1000"));
    ////BOOST_CHECK_EQUAL(result, 0.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__underflow_negative__fails)
{
    double result = 42.0;
    BOOST_CHECK(!accessor::to_number(result, "-1e-1000"));
    ////BOOST_CHECK_EQUAL(result, 0.0);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__huge_integer__fails)
{
    double result = 42.0;
    const std::string huge_integer = "1" + std::string(1000, '0');
    BOOST_CHECK(!accessor::to_number(result, huge_integer));
    ////BOOST_CHECK_EQUAL(result, infinity);
}

BOOST_AUTO_TEST_CASE(parse_json__to_number__huge_negative_integer__fails)
{
    double result = 42.0;
    const std::string huge_integer = "1" + std::string(1000, '0');
    const std::string huge_negative = "-" + huge_integer;
    BOOST_CHECK(!accessor::to_number(result, huge_negative));
    ////BOOST_CHECK_EQUAL(result, -infinity);
}

BOOST_AUTO_TEST_SUITE_END()
