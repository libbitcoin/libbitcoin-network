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

BOOST_AUTO_TEST_SUITE(rpc_request_tests)

using namespace system;
using namespace network::messages::rpc;

BOOST_AUTO_TEST_CASE(rpc_request__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(request::command, "request");
    BOOST_REQUIRE(request::id == identifier::request);
}

BOOST_AUTO_TEST_CASE(rpc_request__size__empty__expected)
{
    const request instance{ verb::post, "", version::http_1_0, {} };
    BOOST_REQUIRE_EQUAL(instance.size(), 18u);
}

BOOST_AUTO_TEST_CASE(rpc_request__size__http_1_1_content_type_json__expected)
{
    const heading::headers_t headers{ { "Content-Type", "application/json" } };
    const request instance{ verb::post, "/api/resource", version::http_1_1, headers };
    BOOST_REQUIRE_EQUAL(instance.size(), 62u);
}

BOOST_AUTO_TEST_CASE(rpc_request__deserialize__empty_request__returns_nullptr)
{
    const data_chunk data{};
    BOOST_REQUIRE(!request::deserialize(data));
}

BOOST_AUTO_TEST_CASE(rpc_request__deserialize__invalid_request__returns_nullptr)
{
    const data_chunk data{ to_chunk("GET /") };
    BOOST_REQUIRE(!request::deserialize(data));
}

BOOST_AUTO_TEST_CASE(rpc_request__serialize___empty__round_trip)
{
    const request original{ verb::options, "", version::http_1_0, {} };

    data_chunk buffer(original.size());
    BOOST_REQUIRE(original.serialize(buffer));

    const auto duplicate = request::deserialize(buffer);
    BOOST_REQUIRE(duplicate);
    BOOST_REQUIRE(duplicate->verb == original.verb);
    BOOST_REQUIRE(duplicate->path == original.path);
    BOOST_REQUIRE(duplicate->version == original.version);
    BOOST_REQUIRE(duplicate->headers == original.headers);
}

// Asserts.
////BOOST_AUTO_TEST_CASE(rpc_request__serialize__insufficient_ostream__returns_nullptr)
////{
////    const heading::headers_t headers{ { "Content-Type", "application/json" }, { "Accept", "text/plain" } };
////    const request instance{ verb::get, "/api/test", version::http_1_1, headers };
////
////    data_chunk buffer(sub1(instance.size()));
////    BOOST_REQUIRE(!instance.serialize(buffer));
////}

BOOST_AUTO_TEST_CASE(rpc_request__serialize__non_empty__round_trip)
{
    const heading::headers_t headers{ { "Content-Type", "application/json" }, { "Accept", "text/plain" } };
    const request original{ verb::get, "/api/test", version::http_1_1, headers };

    data_chunk buffer(original.size());
    BOOST_REQUIRE(original.serialize(buffer));

    const auto duplicate = request::deserialize(buffer);
    BOOST_REQUIRE(duplicate);
    BOOST_REQUIRE(duplicate->verb == original.verb);
    BOOST_REQUIRE(duplicate->path == original.path);
    BOOST_REQUIRE(duplicate->version == original.version);
    BOOST_REQUIRE(duplicate->headers == original.headers);
}

BOOST_AUTO_TEST_CASE(rpc_request__serialize__reader_writer__round_trip)
{
    const heading::headers_t headers{ { "Host", "example.com" } };
    const request original{ verb::post, "/resource", version::http_1_1, headers };

    data_chunk buffer(original.size());
    ostream sink{ buffer };
    byte_writer writer{ sink };
    original.serialize(writer);
    BOOST_REQUIRE(writer);

    istream source{ buffer };
    byte_reader reader{ source };
    const auto duplicate = request::deserialize(reader);
    BOOST_REQUIRE(reader);
    BOOST_REQUIRE(duplicate.verb == original.verb);
    BOOST_REQUIRE(duplicate.path == original.path);
    BOOST_REQUIRE(duplicate.version == original.version);
    BOOST_REQUIRE(duplicate.headers == original.headers);
}

BOOST_AUTO_TEST_CASE(rpc_request__deserialize__string_buffer__expected)
{
    const std::string text{ "GET /api/test HTTP/1.1\r\nContent-Type:application/json\r\nAccept:text/plain\r\n\r\n" };
    const auto instance = request::deserialize(to_chunk(text));
    BOOST_REQUIRE(instance);
    BOOST_REQUIRE(instance->verb == verb::get);
    BOOST_REQUIRE_EQUAL(instance->path, "/api/test");
    BOOST_REQUIRE(instance->version == version::http_1_1);
    BOOST_REQUIRE_EQUAL(instance->headers.size(), 2u);
    BOOST_REQUIRE_EQUAL(instance->headers.find("Content-Type")->second, "application/json");
    BOOST_REQUIRE_EQUAL(instance->headers.find("Accept")->second, "text/plain");
}

BOOST_AUTO_TEST_CASE(rpc_request__serialize__string_buffer__expected)
{
    const std::string expected{ "GET /api/test HTTP/1.1\r\nContent-Type:application/json\r\nAccept:text/plain\r\n\r\n" };
    const heading::headers_t headers{ { "Content-Type", "application/json" }, { "Accept", "text/plain" } };
    const request instance{ verb::get, "/api/test", version::http_1_1, headers };

    data_chunk buffer(instance.size());
    BOOST_REQUIRE(instance.serialize(buffer));
    BOOST_REQUIRE_EQUAL(to_string(buffer), expected);
}

BOOST_AUTO_TEST_SUITE_END()
