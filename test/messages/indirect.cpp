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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(indirect_tests)

BOOST_AUTO_TEST_CASE(indirect__construct__value__expected)
{
    const indirect<std::string> instance{ std::string{ "value" } };
    BOOST_REQUIRE_EQUAL(*instance, "value");
    BOOST_REQUIRE_EQUAL(instance->size(), 5u);
}

BOOST_AUTO_TEST_CASE(indirect__copy__distinct_values)
{
    indirect<std::string> instance{ std::string{ "value" } };
    const auto copy = instance;
    *instance = "changed";
    BOOST_REQUIRE_EQUAL(*copy, "value");
    BOOST_REQUIRE_EQUAL(*instance, "changed");
}

BOOST_AUTO_TEST_CASE(indirect__copy_assign__distinct_values)
{
    indirect<std::string> instance{ std::string{ "value" } };
    indirect<std::string> other{ std::string{ "other" } };
    other = instance;
    *instance = "changed";
    BOOST_REQUIRE_EQUAL(*other, "value");
}

BOOST_AUTO_TEST_CASE(indirect__move__same_storage)
{
    indirect<std::string> instance{ std::string{ "value" } };
    const auto address = &*instance;
    const auto moved = std::move(instance);
    BOOST_REQUIRE_EQUAL(&*moved, address);
    BOOST_REQUIRE_EQUAL(*moved, "value");
}

BOOST_AUTO_TEST_CASE(indirect__body_value__largest_inline_kind)
{
    BOOST_REQUIRE_EQUAL(sizeof(http::body_value), sizeof(std::variant<http::peer_value>));
}

BOOST_AUTO_TEST_CASE(indirect__body__rpc_request__contained)
{
    http::body::value_type body{};
    body = rpc::request{ .batch = true };
    BOOST_REQUIRE(body.contains<rpc::request>());
    BOOST_REQUIRE(!body.contains<rpc::response>());
    BOOST_REQUIRE(body.get<rpc::request>().batch);
}

BOOST_AUTO_TEST_SUITE_END()
