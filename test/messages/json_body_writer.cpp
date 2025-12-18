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

#if defined(HAVE_SLOW_TESTS)

BOOST_AUTO_TEST_SUITE(json_body_writer_tests)

using namespace network::http;
using namespace network::json;
using value = boost::json::value;
using object = boost::json::object;

bool operator==(const asio::const_buffer& left, const asio::const_buffer& right)
{
    return left.size() == right.size() &&
        is_zero(std::memcmp(left.data(), right.data(), left.size()));
}

bool operator!=(const asio::const_buffer& left, const asio::const_buffer& right)
{
    return !(left == right);
}

BOOST_AUTO_TEST_CASE(json_body_writer__constructor1__default__null_model)
{
    json::body::value_type body{};
    json::body::writer writer(body);
    BOOST_REQUIRE(boost::get<value>(body.model).is_null());
}

BOOST_AUTO_TEST_CASE(json_body_writer__constructor2__default__null_model)
{
    response_header header{};
    json::body::value_type body{};
    json::body::writer writer(header, body);
    BOOST_REQUIRE(boost::get<value>(body.model).is_null());
}

BOOST_AUTO_TEST_CASE(json_body_writer__init__default__success)
{
    json::body::value_type body{};
    json::body::writer writer(body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(json_body_writer__get__null_model__success_expected_no_more)
{
    const std::string_view expected{ "null" };
    const asio::const_buffer out{ expected.data(), expected.size() };
    json::body::value_type body{};
    json::body::writer writer(body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer.has_value());
    BOOST_REQUIRE(buffer.get().first == out);
    BOOST_REQUIRE(!buffer.get().second);
}

BOOST_AUTO_TEST_CASE(json_body_writer__get__simple_object__success_expected_no_more)
{
    const std::string_view expected{ R"({"key":"value"})" };
    const asio::const_buffer out{ expected.data(), expected.size() };
    json::body::value_type body{};
    body.model = object{ { "key", "value" } };
    json::body::writer writer(body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer.has_value());
    BOOST_REQUIRE(buffer.get().first == out);
    BOOST_REQUIRE(!buffer.get().second);
}

BOOST_AUTO_TEST_SUITE_END()

#endif // HAVE_SLOW_TESTS
