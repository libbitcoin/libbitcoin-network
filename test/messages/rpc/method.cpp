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

BOOST_AUTO_TEST_SUITE(method_tests)

using namespace network::http::method;

BOOST_AUTO_TEST_CASE(method__static_method__always__equals_verb)
{
    BOOST_REQUIRE(get::method == http::verb::get);
    BOOST_REQUIRE(head::method == http::verb::head);
    BOOST_REQUIRE(post::method == http::verb::post);
    BOOST_REQUIRE(put::method == http::verb::put);
    BOOST_REQUIRE(delete_::method == http::verb::delete_);
    BOOST_REQUIRE(trace::method == http::verb::trace);
    BOOST_REQUIRE(options::method == http::verb::options);
    BOOST_REQUIRE(connect::method == http::verb::connect);
    BOOST_REQUIRE(unknown::method == http::verb::unknown);
}

BOOST_AUTO_TEST_CASE(method__bool_operator__default__null_and_false)
{
    const get instance{};
    BOOST_REQUIRE(!instance);
    BOOST_REQUIRE(!instance.ptr);
}

BOOST_AUTO_TEST_CASE(method__arrow_operator__non_null__expected)
{
    constexpr auto expected_method = http::verb::get;
    const get instance{ std::make_shared<http::string_request>(expected_method, "/", 11) };
    BOOST_REQUIRE(instance->method() == expected_method);
}

BOOST_AUTO_TEST_CASE(method__asterisk_operator__non_null__expected)
{
    constexpr auto expected_method = http::verb::post;
    const get instance{ std::make_shared<http::string_request>(expected_method, "/", 11) };
    BOOST_REQUIRE((*instance).method() == expected_method);
}

BOOST_AUTO_TEST_SUITE_END()
