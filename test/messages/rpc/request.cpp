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

// properties

BOOST_AUTO_TEST_CASE(rpc_request__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(request::command, "request");
    BOOST_REQUIRE(request::id == identifier::request);
}

// size

BOOST_AUTO_TEST_CASE(rpc_request__size__empty__expected)
{
    const request instance{ method::post, "", version::http_1_0, {} };
    BOOST_REQUIRE_EQUAL(instance.size(), 18u);
}

BOOST_AUTO_TEST_CASE(rpc_request__size__http_1_1_content_type_json__expected)
{
    const heading::fields fields{ { "Content-Type", "application/json" } };
    const request instance{ method::post, "/api/resource", version::http_1_1, fields };
    BOOST_REQUIRE_EQUAL(instance.size(), 63u);
}

// target

BOOST_AUTO_TEST_CASE(rpc_request__target__origin_form__returns_origin)
{
    const request instance{ method::get, "/index.html?query=example", version::http_1_1, {} };
    BOOST_REQUIRE(instance.target() == target::origin);
}

BOOST_AUTO_TEST_CASE(rpc_request__target__absolute_form__returns_absolute)
{
    const request instance{ method::post, "https://example.com/path", version::http_1_0, {} };
    BOOST_REQUIRE(instance.target() == target::absolute);
}

BOOST_AUTO_TEST_CASE(rpc_request__target__authority_form__returns_undefined)
{
    // Implementation incomplete.
    const request instance{ method::connect, "example.com:443", version::http_0_9, {} };
    BOOST_REQUIRE(instance.target() == target::undefined);
}

BOOST_AUTO_TEST_CASE(rpc_request__target__asterisk_form__returns_asterisk)
{
    const request instance{ method::options, "*", version::http_1_1, {} };
    BOOST_REQUIRE(instance.target() == target::asterisk);
}

BOOST_AUTO_TEST_CASE(rpc_request__target__invalid_path__returns_undefined)
{
    const request instance{ method::get, "invalid", version::http_1_1, {} };
    BOOST_REQUIRE(instance.target() == target::undefined);
}

// valid

BOOST_AUTO_TEST_CASE(rpc_request__valid__valid_request__returns_true)
{
    const request instance{ method::get, "/path", version::http_1_1, heading::fields{ { "host", "example.com" } } };
    BOOST_REQUIRE(instance.valid());
}

BOOST_AUTO_TEST_CASE(rpc_request__valid__undefined_method__returns_false)
{
    const request instance{ method::undefined, "/path", version::http_1_1, {} };
    BOOST_REQUIRE(!instance.valid());
}

BOOST_AUTO_TEST_CASE(rpc_request__valid__undefined_version__returns_false)
{
    const request instance{ method::get, "/path", version::undefined, {} };
    BOOST_REQUIRE(!instance.valid());
}

BOOST_AUTO_TEST_CASE(rpc_request__valid__undefined_target__returns_false)
{
    const request instance{ method::get, "invalid", version::http_1_1, {} };
    BOOST_REQUIRE(!instance.valid());
}

// deserialize

BOOST_AUTO_TEST_CASE(rpc_request__deserialize__valid_request__returns_valid)
{
    std::istringstream in("GET /index.html HTTP/1.0\r\nHost: example.com\r\n\r\n");
    read::bytes::istream reader(in);
    const auto instance = request::deserialize(reader);
    BOOST_REQUIRE(instance.valid());
    BOOST_REQUIRE(instance.method == method::get);
    BOOST_REQUIRE(instance.version == version::http_1_0);
    BOOST_REQUIRE_EQUAL(instance.path, "/index.html");
    BOOST_REQUIRE_EQUAL(instance.fields.size(), one);
    BOOST_REQUIRE_EQUAL(instance.fields.begin()->first, "host");
    BOOST_REQUIRE_EQUAL(instance.fields.begin()->second, "example.com");
}

BOOST_AUTO_TEST_CASE(rpc_request__deserialize__invalid_target__returns_invalid)
{
    std::istringstream in("GET invalid HTTP/1.1\r\nHost: example.com\r\n\r\n");
    read::bytes::istream reader(in);
    const auto instance = request::deserialize(reader);
    BOOST_REQUIRE(!instance.valid());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_request__deserialize__invalid_method__returns_invalid)
{
    std::istringstream in("INVALID /path HTTP/1.1\r\nHost: example.com\r\n\r\n");
    read::bytes::istream reader(in);
    const auto instance = request::deserialize(reader);
    BOOST_REQUIRE(!instance.valid());
    BOOST_CHECK(!reader);
}

BOOST_AUTO_TEST_CASE(rpc_request__deserialize__connect_slash__returns_invalid)
{
    std::istringstream in("CONNECT /example.com HTTP/1.1\r\nHost: example.com\r\n\r\n");
    read::bytes::istream reader(in);
    const auto instance = request::deserialize(reader);
    BOOST_REQUIRE(!instance.valid());
    BOOST_CHECK(!reader);
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
    const request original{ method::head, "/", version::http_1_0, {} };

    data_chunk buffer(original.size());
    BOOST_REQUIRE(original.serialize(buffer));

    const auto duplicate = request::deserialize(buffer);
    BOOST_REQUIRE(duplicate);
    BOOST_REQUIRE(duplicate->valid());
    BOOST_REQUIRE(duplicate->target() == target::origin);
    BOOST_REQUIRE(duplicate->method == original.method);
    BOOST_REQUIRE(duplicate->path == original.path);
    BOOST_REQUIRE(duplicate->version == original.version);
    BOOST_REQUIRE(duplicate->fields == original.fields);
}

// serialize

BOOST_AUTO_TEST_CASE(rpc_request__serialize__non_empty__round_trip)
{
    const heading::fields fields{ { "Content-Type", "application/json" }, { "Accept", "text/plain" } };
    const heading::fields lowered{ { "content-type", "application/json" }, { "accept", "text/plain" } };
    const request original{ method::get, "/api/test", version::http_1_1, fields };

    data_chunk buffer(original.size());
    BOOST_REQUIRE(original.serialize(buffer));

    const auto duplicate = request::deserialize(buffer);
    BOOST_REQUIRE(duplicate);
    BOOST_REQUIRE(duplicate->method == original.method);
    BOOST_REQUIRE(duplicate->path == original.path);
    BOOST_REQUIRE(duplicate->version == original.version);
    BOOST_REQUIRE(duplicate->fields == lowered);
}

BOOST_AUTO_TEST_CASE(rpc_request__serialize__reader_writer__round_trip)
{
    const heading::fields fields{ { "Host", "example.com" } };
    const heading::fields lowered{ { "host", "example.com" } };
    const request original{ method::post, "/resource", version::http_1_1, fields };

    data_chunk buffer(original.size());
    ostream sink{ buffer };
    byte_writer writer{ sink };
    original.serialize(writer);
    BOOST_REQUIRE(writer);

    istream source{ buffer };
    byte_reader reader{ source };
    const auto duplicate = request::deserialize(reader);
    BOOST_REQUIRE(reader);
    BOOST_REQUIRE(duplicate.method == original.method);
    BOOST_REQUIRE(duplicate.path == original.path);
    BOOST_REQUIRE(duplicate.version == original.version);
    BOOST_REQUIRE(duplicate.fields == lowered);
}

BOOST_AUTO_TEST_CASE(rpc_request__deserialize__string_buffer__expected)
{
    const std::string text{ "GET /api/test HTTP/1.1\r\nContent-Type:application/json\r\nAccept:text/plain\r\n\r\n" };
    const auto instance = request::deserialize(to_chunk(text));
    BOOST_REQUIRE(instance);
    BOOST_REQUIRE(instance->method == method::get);
    BOOST_REQUIRE_EQUAL(instance->path, "/api/test");
    BOOST_REQUIRE(instance->version == version::http_1_1);
    BOOST_REQUIRE_EQUAL(instance->fields.size(), 2u);
    BOOST_REQUIRE_EQUAL(instance->fields.find("content-type")->second, "application/json");
    BOOST_REQUIRE_EQUAL(instance->fields.find("accept")->second, "text/plain");
}

BOOST_AUTO_TEST_CASE(rpc_request__serialize__string_buffer__expected)
{
    // Use of std::multimap (ordered) sorts fields.
    const std::string expected{ "GET /api/test HTTP/1.1\r\naccept: text/plain\r\ncontent-type: application/json\r\n\r\n" };
    const heading::fields fields{ { "content-type", "application/json" }, { "accept", "text/plain" } };
    const request instance{ method::get, "/api/test", version::http_1_1, fields };

    data_chunk buffer(instance.size());
    BOOST_REQUIRE(instance.serialize(buffer));
    BOOST_REQUIRE_EQUAL(to_string(buffer), expected);
}

BOOST_AUTO_TEST_CASE(rpc_request__deserialize__big_buffer__expected_trimmed)
{
    const std::string text
    {
        "GET / HTTP/1.1\r\n"
        "Host:  192.168.0.219:8080\t \r\n"
        "Connection: keep-alive\r\n"
        "Upgrade-Insecure-Requests: 1\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36\r\n"
        "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Accept-Language: en-US,en;q=0.9\r\n"
        "\r\n"
    };

    const auto buffer = to_chunk(text);
    istream source{ buffer };
    byte_reader reader{ source };
    reader.set_limit(buffer.size());
    const auto instance = to_shared(request::deserialize(reader));
    BOOST_REQUIRE(reader);
    BOOST_REQUIRE(instance->method == method::get);
    BOOST_REQUIRE_EQUAL(instance->path, "/");
    BOOST_REQUIRE(instance->version == version::http_1_1);
    BOOST_REQUIRE_EQUAL(instance->fields.size(), 7u);
    BOOST_REQUIRE_EQUAL(instance->fields.find("accept")->second, "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
    BOOST_REQUIRE_EQUAL(instance->fields.find("accept-encoding")->second, "gzip, deflate");
    BOOST_REQUIRE_EQUAL(instance->fields.find("accept-language")->second, "en-US,en;q=0.9");
    BOOST_REQUIRE_EQUAL(instance->fields.find("connection")->second, "keep-alive");
    BOOST_REQUIRE_EQUAL(instance->fields.find("host")->second, "192.168.0.219:8080");
    BOOST_REQUIRE_EQUAL(instance->fields.find("upgrade-insecure-requests")->second, "1");
    BOOST_REQUIRE_EQUAL(instance->fields.find("user-agent")->second, "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/140.0.0.0 Safari/537.36");
}

BOOST_AUTO_TEST_SUITE_END()
