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

BOOST_AUTO_TEST_SUITE(serializer_tests)

// serializer
// ----------------------------------------------------------------------------

template <size_t Size>
bool write_chunk(std::ostream& capture, boost::json::serializer& serial)
{
    size_t total{};
    std::array<char, Size> buffer{};

    // fill up the outgoing buffer.
    while (total < Size && !serial.done())
    {
        const auto start = std::next(buffer.data(), total);
        const auto bytes = Size - total;
        total += serial.read(start, bytes).size();
    }

    // TODO: make these real.
    boost::asio::io_context context{};
    boost::asio::ip::tcp::socket socket{ context };
    const boost::asio::const_buffer view{ buffer.data(), total };

    // testing only, simulates async write.
    ///////////////////////////////////////////////////////////////////////
    capture.write(static_cast<const char*>(view.data()),
        static_cast<std::streamsize>(total));
    ///////////////////////////////////////////////////////////////////////

    // TODO: async, partial write allowed.
    // write the buffer to the socket.
    ////boost::asio::write(socket, view);
    return !capture.bad();
}

template <size_t Size>
bool stream_chunk(std::ostream& out, boost::json::serializer& serial)
{
    size_t total{};
    std::array<char, Size> buffer{};

    // fill up the outgoing buffer.
    while (total < Size && !serial.done())
    {
        const auto start = std::next(buffer.data(), total);
        const auto bytes = Size - total;

        // partial read allowed.
        total += serial.read(start, bytes).size();
    }

    // write the buffer to the stream.
    out.write(buffer.data(), static_cast<std::streamsize>(total));
    return !out.bad();
}

BOOST_AUTO_TEST_CASE(serializer_chunk_test)
{
    const auto expected = R"({"name":"Boost.JSON","version":"1.86"})";

    std::ostringstream out{};
    boost::json::serializer serial{};
    const auto model = boost::json::parse(expected);
    ////const boost::json::value model
    ////{
    ////    { "name", "Boost.JSON" },
    ////    { "version", "1.86" }
    ////};

    serial.reset(&model);
    while (!serial.done())
    {
        BOOST_REQUIRE(stream_chunk<16>(out, serial));
    }

    BOOST_REQUIRE_EQUAL(out.str(), expected);

    out.clear();
    out.str({});
    serial.reset(&model);
    while (!serial.done())
    {
        BOOST_REQUIRE(write_chunk<16>(out, serial));
    }

    BOOST_REQUIRE_EQUAL(out.str(), expected);
}

// parser
// ----------------------------------------------------------------------------

template <size_t Size>
bool read_chunk(std::istream& capture, boost::json::stream_parser& parse)
{
    size_t total{};
    std::array<char, Size> buffer{};

    // TODO: make these real.
    boost::asio::io_context context{};
    boost::asio::ip::tcp::socket socket{ context };

    // fill up the buffer from the socket.
    while (total < Size && !capture.eof())
    {
        const auto start = std::next(buffer.data(), total);
        const auto bytes = Size - total;
        const boost::asio::mutable_buffer view{ start, bytes };

        // testing only, simulates async read.
        ///////////////////////////////////////////////////////////////////////
        capture.read(start, static_cast<std::streamsize>(bytes));
        total += static_cast<size_t>(capture.gcount());
        ///////////////////////////////////////////////////////////////////////

        // TODO: async, partial read allowed.
        ////total += boost::asio::read(socket, view);
    }

    // partial write allowed.
    // parse the incoming buffer.
    boost::system::error_code ec{};
    parse.write_some(buffer.data(), total, ec);
    return !ec && !capture.bad();
}

template <size_t Size>
bool stream_chunk(std::istream& source, boost::json::stream_parser& parse)
{
    size_t total{};
    std::array<char, Size> buffer{};

    // fill up the buffer from the stream.
    while (total < Size && !source.eof())
    {
        const auto start = buffer.data() + total;
        const auto bytes = Size - total;
        source.read(start, static_cast<std::streamsize>(bytes));
        total += static_cast<size_t>(source.gcount());
    }

    // partial write allowed.
    // parse the incoming buffer.
    boost::system::error_code ec{};
    parse.write_some(buffer.data(), total, ec);
    return !ec && !source.bad();
}

BOOST_AUTO_TEST_CASE(parser_chunk_test)
{
    const auto text = R"({"name":"Boost.JSON","version":"1.86"})";
    const auto expected = boost::json::parse(text);
    ////const boost::json::value expected
    ////{
    ////    { "name", "Boost.JSON" },
    ////    { "version", "1.86" }
    ////};

    std::istringstream in{ text };
    boost::json::stream_parser parser{};

    while (!parser.done())
    {
        BOOST_REQUIRE(stream_chunk<16>(in, parser));
    }

    BOOST_REQUIRE_EQUAL(parser.release(), expected);

    in.clear();
    in.str(text);
    parser.reset();
    while (!parser.done())
    {
        BOOST_REQUIRE(read_chunk<16>(in, parser));
    }

    BOOST_REQUIRE_EQUAL(parser.release(), expected);
}

using namespace network::json;

// non-strict until tests are updated for "method" non-empty required.
using request_parser = parser<false, version::any, false>;

BOOST_AUTO_TEST_CASE(serializer__write_request__deserialized__expected)
{
    // not valid json, testing blob parser.
    const string_t text
    {
        R"({)"
            R"("jsonrpc":"2.0",)"
            R"("id":-42,)"
            R"("method":"random",)"
            R"("params":)"
            R"({)"
                R"("array":[A],)"
                R"("false":false,)"
                R"("foo":"bar",)"
                R"("null":null,)"
                R"("number":42,)"
                R"("object":{O},)"
                R"("true":true)"
            R"(})"
        R"(})"
    };

    request_parser parse{};
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(parse);

    // params are sorted by serializer, so must be above as well.
    BOOST_REQUIRE_EQUAL(serializer<request_t>::write(parse.get()), text);
}

BOOST_AUTO_TEST_CASE(serializer__write_request__nested_terminators__expected)
{
    // not valid json, testing blob parser.
    const string_t text
    {
        R"({)"
            R"("jsonrpc":"2.0",)"
            R"("params":)"
            R"({)"
                R"("array":[aaa"]"bbb],)"
                R"("object":{aaa"}"bbb})"
            R"(})"
        R"(})"
    };

    request_parser parse{};
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(parse);
    BOOST_REQUIRE_EQUAL(serializer<request_t>::write(parse.get()), text);
}

BOOST_AUTO_TEST_CASE(serializer__write_request__nested_escapes__expected)
{
    // not valid json, testing blob parser.
    const string_t text
    {
        R"({)"
            R"("jsonrpc":"2.0",)"
            R"("params":)"
            R"({)"
                R"("array":[aaa"\"\\"bbb],)"
                R"("object":{aaa"\"\\"bbb})"
            R"(})"
        R"(})"
    };

    request_parser parse{};
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(parse);
    BOOST_REQUIRE_EQUAL(serializer<request_t>::write(parse.get()), text);
}

BOOST_AUTO_TEST_CASE(serializer__write_request__nested_containers__expected)
{
    // not valid json, testing blob parser.
    const string_t text
    {
        R"({)"
            R"("jsonrpc":"2.0",)"
            R"("params":)"
            R"({)"
                R"("array":[{}{{}}{{{}}}[[[]]][[]][]],)"
                R"("object":{[[[]]][[]][]{}{{}}{{{}}}})"
            R"(})"
        R"(})"
    };

    request_parser parse{};
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(parse);
    BOOST_REQUIRE_EQUAL(serializer<request_t>::write(parse.get()), text);
}

BOOST_AUTO_TEST_CASE(serializer__serialize__simple_result__expected)
{
    response_t response;
    response.jsonrpc = version::v2;
    response.id = identity_t{ code_t{ 42 } };
    response.result = value_t{ std::in_place_type<number_t>, number_t{ 100.5 } };

    const auto json = serializer<response_t>::write(response);
    BOOST_CHECK_EQUAL(json, R"({"jsonrpc":"2.0","id":42,"result":100.5})");
}

BOOST_AUTO_TEST_CASE(serializer__serialize__error_response__expected)
{
    response_t response;
    response.jsonrpc = version::v2;
    response.id = identity_t{ string_t{ "abc123" } };
    response.error = result_t{ -32602, "Invalid params", {} };

    const auto text = serializer<response_t>::write(response);
    BOOST_CHECK_EQUAL(text, R"({"jsonrpc":"2.0","id":"abc123","error":{"code":-32602,"message":"Invalid params"}})");
}

BOOST_AUTO_TEST_CASE(serializer__serialize__error_with_data__expected)
{
    response_t response;
    response.jsonrpc = version::v1;
    response.id = identity_t{ null_t{} };
    response.error = result_t{ -32700, "Parse error", value_t{std::in_place_type<string_t>, string_t{ "Invalid JSON" }} };

    const auto text = serializer<response_t>::write(response);
    BOOST_CHECK_EQUAL(text, R"({"jsonrpc":"1.0","id":null,"error":{"code":-32700,"message":"Parse error","data":"Invalid JSON"}})");
}

BOOST_AUTO_TEST_CASE(serializer__serialize__empty_result__expected)
{
    response_t response;
    response.jsonrpc = version::v2;
    response.id = identity_t{ code_t{} };
    response.result = value_t{ std::in_place_type<null_t>, null_t{} };

    const auto text = serializer<response_t>::write(response);
    BOOST_CHECK_EQUAL(text, R"({"jsonrpc":"2.0","id":0,"result":null})");
}

BOOST_AUTO_TEST_SUITE_END()
