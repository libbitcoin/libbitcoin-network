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

BOOST_AUTO_TEST_SUITE(json_body_writer_tests)

using namespace network::http;
using namespace network::json;
using namespace network::error;
using body = json::body<parser, serializer>;
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

BOOST_AUTO_TEST_CASE(json_body_writer__constructor__default__null_model)
{
    payload body{};
    response_header header{};
    body::writer writer(header, body);
    BOOST_REQUIRE(boost::get<value>(body.model).is_null());
}

BOOST_AUTO_TEST_CASE(json_body_writer__init__default__success)
{
    payload body{};
    response_header header{};
    body::writer writer(header, body);
    error_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(json_body_writer__get__null_model__success_expected_no_more)
{
    const std::string_view expected{ "null" };
    const asio::const_buffer out{ expected.data(), expected.size() };
    payload body{};
    response_header header{};
    body::writer writer(header, body);
    error_code ec{};
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
    payload body{};
    body.model = object{ { "key", "value" } };
    response_header header{};
    body::writer writer(header, body);
    error_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer.has_value());
    BOOST_REQUIRE(buffer.get().first == out);
    BOOST_REQUIRE(!buffer.get().second);
}

BOOST_AUTO_TEST_SUITE_END()
