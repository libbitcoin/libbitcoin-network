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

BOOST_AUTO_TEST_SUITE(json_body_reader_tests)

using namespace network::http;
using namespace network::json;
using namespace network::error;
using body = json::body<json::parser, json::serializer>;
using value = boost::json::value;

BOOST_AUTO_TEST_CASE(json_body_reader__construct__default__null_model)
{
    json::payload body{};
    request_header header{};
    body::reader reader(header, body);
    BOOST_REQUIRE(boost::get<value>(body.model).is_null());
}

BOOST_AUTO_TEST_CASE(json_body_reader__init__simple_object__success)
{
    const std::string_view text{ R"({"key":"value"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    json::payload body{};
    request_header header{};
    body::reader reader(header, body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(json_body_reader__put__simple_object__success_expected_consumed)
{
    const std::string_view text{ R"({"key":"value"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    json::payload body{};
    request_header header{};
    body::reader reader(header, body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(json_body_reader__finish__simple_object__success_expected_model)
{
    const std::string_view text{ R"({"key":"value"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };

    json::payload body{};
    request_header header{};
    body::reader reader(header, body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.model.is_object());
    BOOST_REQUIRE(body.model.as_object().contains("key"));
    BOOST_REQUIRE_EQUAL(body.model.as_object()["key"].as_string(), "value");
}

BOOST_AUTO_TEST_CASE(json_body_reader__put__over_length__protocol_error)
{
    const std::string_view text{ R"({"key":"value"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    json::payload body{};
    request_header header{};
    body::reader reader(header, body);
    error_code ec{};
    reader.init(10, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE_EQUAL(ec, boost_error_t::protocol_error);
}

BOOST_AUTO_TEST_SUITE_END()
