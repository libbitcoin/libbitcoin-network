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

BOOST_AUTO_TEST_SUITE(rpc_response_tests)

using namespace system;
using namespace network::messages::rpc;

BOOST_AUTO_TEST_CASE(rpc_response__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(response::command, "response");
    BOOST_REQUIRE(response::id == identifier::response);
}

BOOST_AUTO_TEST_CASE(rpc_response__size__empty__expected)
{
    const response instance{ version::http_1_0, status::ok, {} };
    BOOST_REQUIRE_EQUAL(instance.size(), 19u);
}

BOOST_AUTO_TEST_CASE(rpc_response__size__http_1_1_content_type_json__expected)
{
    const heading::headers_t headers{ { "Content-Type", "application/json" } };
    const response instance{ version::http_1_1, status::ok, headers };
    BOOST_REQUIRE_EQUAL(instance.size(), 50u);
}

BOOST_AUTO_TEST_CASE(rpc_response__deserialize__empty_response__returns_nullptr)
{
    const data_chunk data{};
    BOOST_REQUIRE(!response::deserialize(data));
}

BOOST_AUTO_TEST_CASE(rpc_response__deserialize__invalid_response__returns_nullptr)
{
    const data_chunk data{ to_chunk("HTTP/1.1") }; // Missing status and terminal
    BOOST_REQUIRE(!response::deserialize(data));
}

BOOST_AUTO_TEST_CASE(rpc_response__serialize__empty__round_trip)
{
    const response original{ version::http_1_0, status::no_content, {} };

    data_chunk buffer(original.size());
    BOOST_REQUIRE(original.serialize(buffer));

    const auto duplicate = response::deserialize(buffer);
    BOOST_REQUIRE(duplicate);
    BOOST_REQUIRE(duplicate->version == original.version);
    BOOST_REQUIRE(duplicate->status == original.status);
    BOOST_REQUIRE(duplicate->headers == original.headers);
}

// Asserts.
////BOOST_AUTO_TEST_CASE(rpc_response__serialize__insufficient_ostream__returns_nullptr)
////{
////    const heading::headers_t headers{ { "Content-Type", "application/json" }, { "Accept", "text/plain" } };
////    const response instance{ version::http_1_1, status::ok, headers };
////
////    data_chunk buffer(sub1(instance.size()));
////    BOOST_REQUIRE(!instance.serialize(buffer));
////}

BOOST_AUTO_TEST_CASE(rpc_response__serialize__non_empty__round_trip)
{
    const heading::headers_t headers{ { "Content-Type", "application/json" }, { "Accept", "text/plain" } };
    const response original{ version::http_1_1, status::ok, headers };

    data_chunk buffer(original.size());
    BOOST_REQUIRE(original.serialize(buffer));

    const auto duplicate = response::deserialize(buffer);
    BOOST_REQUIRE(duplicate);
    BOOST_REQUIRE(duplicate->version == original.version);
    BOOST_REQUIRE(duplicate->status == original.status);
    BOOST_REQUIRE(duplicate->headers == original.headers);
}

BOOST_AUTO_TEST_CASE(rpc_response__serialize__reader_writer__round_trip)
{
    const heading::headers_t headers{ { "Host", "example.com" } };
    const response original{ version::http_1_1, status::created, headers };

    data_chunk buffer(original.size());
    ostream sink{ buffer };
    byte_writer writer{ sink };
    original.serialize(writer);
    BOOST_REQUIRE(writer);

    istream source{ buffer };
    byte_reader reader{ source };
    const auto duplicate = response::deserialize(reader);
    BOOST_REQUIRE(reader);
    BOOST_REQUIRE(duplicate.version == original.version);
    BOOST_REQUIRE(duplicate.status == original.status);
    BOOST_REQUIRE(duplicate.headers == original.headers);
}

BOOST_AUTO_TEST_CASE(rpc_response__deserialize__string_buffer__expected)
{
    const std::string text{ "HTTP/1.1 200 OK\r\nContent-Type:application/json\r\nAccept:text/plain\r\n\r\n" };
    const auto instance = response::deserialize(to_chunk(text));
    BOOST_REQUIRE(instance);
    BOOST_REQUIRE(instance->version == version::http_1_1);
    BOOST_REQUIRE(instance->status == status::ok);
    BOOST_REQUIRE_EQUAL(instance->headers.size(), 2u);
    BOOST_REQUIRE_EQUAL(instance->headers.find("Content-Type")->second, "application/json");
    BOOST_REQUIRE_EQUAL(instance->headers.find("Accept")->second, "text/plain");
}

BOOST_AUTO_TEST_CASE(rpc_response__serialize__string_buffer__expected)
{
    // Use of std::multimap (ordered) sorts headers.
    const std::string expected{ "HTTP/1.1 200 OK\r\nAccept:text/plain\r\nContent-Type:application/json\r\n\r\n" };
    const heading::headers_t headers{ { "Content-Type", "application/json" }, { "Accept", "text/plain" } };
    const response instance{ version::http_1_1, status::ok, headers };

    data_chunk buffer(instance.size());
    BOOST_REQUIRE(instance.serialize(buffer));
    BOOST_REQUIRE_EQUAL(to_string(buffer), expected);
}

BOOST_AUTO_TEST_SUITE_END()
