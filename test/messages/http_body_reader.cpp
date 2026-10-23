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

////#if defined(HAVE_SLOW_TESTS)

using namespace http;
using namespace network::http;

struct accessor
  : public body::reader
{
    // Forwarding constructor required because base is explicit and templated.
    template <bool IsRequest, class Fields>
    accessor(http::message_header<IsRequest, Fields>& header,
        body::value_type& value, bool streamed=false) NOEXCEPT
      : base(header, value, streamed)
    {
    }

    using base = body::reader;
    using base::reader_;
};

BOOST_AUTO_TEST_SUITE(http_body_reader_tests)

BOOST_AUTO_TEST_CASE(http_body_reader__init__default_body_bogus__empty_reader)
{
    message_header<true, fields> header{};
    header.set(http::field::content_type, "bogus");
    body::value_type value{};
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(std::holds_alternative<empty_reader>(reader.reader_));
}

BOOST_AUTO_TEST_CASE(http_body_reader__init__string_body_bogus__string_reader)
{
    message_header<true, fields> header{};
    header.set(http::field::content_type, "bogus");
    body::value_type value{};
    value = string_body::value_type{};
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(std::holds_alternative<string_reader>(reader.reader_));
}

BOOST_AUTO_TEST_CASE(http_body_reader__init__default_body_plain_application_json__constructs_json_reader)
{
    message_header<true, fields> header{};
    header.set(http::field::content_type, "application/json");
    body::value_type value{};
    value.plain_json = true;
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(std::holds_alternative<json_reader>(reader.reader_));
}

BOOST_AUTO_TEST_CASE(http_body_reader__init__default_body_not_plain_application_json__constructs_rpc_reader)
{
    message_header<true, fields> header{};
    header.set(http::field::content_type, "application/json");
    body::value_type value{};
    value.plain_json = false;
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(std::holds_alternative<rpc::reader>(reader.reader_));
}

BOOST_AUTO_TEST_CASE(http_body_reader__init__default_body_application_octet_stream__constructs_data_reader)
{
    message_header<true, fields> header{};
    header.set(http::field::content_type, "application/octet-stream");
    header.set(http::field::content_disposition, "bogus");
    body::value_type value{};
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(std::holds_alternative<data_reader>(reader.reader_));
}

BOOST_AUTO_TEST_CASE(http_body_reader__init__default_body_application_octet_stream_with_attachment__file_reader_bad_file_descriptor)
{
    message_header<true, fields> header{};
    header.set(http::field::content_type, "application/octet-stream");
    header.set(http::field::content_disposition, "filename=somenonsense.jpg");
    body::value_type value{};
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(std::holds_alternative<file_reader>(reader.reader_));
    BOOST_REQUIRE(ec == error::to_errc_code(error::boost_errc_t::bad_file_descriptor));
}

BOOST_AUTO_TEST_CASE(http_body_reader__init__default_body_application_octet_stream_with_dirty_attachment__file_reader_bad_file_descriptor)
{
    message_header<true, fields> header{};
    header.set(http::field::content_type, "application/octet-stream");
    header.set(http::field::content_disposition, "dirty 42; filename* = somenonsense.jpg; some other nonsense");
    body::value_type value{};
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(std::holds_alternative<file_reader>(reader.reader_));
    BOOST_REQUIRE(ec == error::to_errc_code(error::boost_errc_t::bad_file_descriptor));
}

BOOST_AUTO_TEST_CASE(http_body_reader__init__default_body_text_plain__constructs_string_reader)
{
    message_header<true, fields> header{};
    header.set(http::field::content_type, "text/plain");
    body::value_type value{};
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(std::holds_alternative<string_reader>(reader.reader_));
}

BOOST_AUTO_TEST_CASE(http_body_reader__finish__complete_rpc__success)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"x","params":[]})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    message_header<true, fields> header{};
    header.set(http::field::content_type, "application/json");
    body::value_type value{};
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
}

// Beast finishes a complete body, so need_more would read past it.
BOOST_AUTO_TEST_CASE(http_body_reader__finish__incomplete_rpc__incomplete)
{
    const std::string_view text{ R"({"id":1)" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    message_header<true, fields> header{};
    header.set(http::field::content_type, "application/json");
    body::value_type value{};
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(ec == error::to_json_code(error::json_error_t::json_incomplete));
}

BOOST_AUTO_TEST_CASE(http_body_reader__finish__number_rpc__incomplete)
{
    const std::string_view text{ "1" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    message_header<true, fields> header{};
    header.set(http::field::content_type, "application/json");
    body::value_type value{};
    accessor reader(header, value);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(ec == error::to_json_code(error::json_error_t::json_incomplete));
}

// A streamed read may finish before the message is complete.
BOOST_AUTO_TEST_CASE(http_body_reader__finish__incomplete_rpc_streamed__need_more)
{
    const std::string_view text{ R"({"id":1)" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    message_header<true, fields> header{};
    header.set(http::field::content_type, "application/json");
    body::value_type value{};
    accessor reader(header, value, true);
    length_type length{ max_size_t };
    boost_code ec{};
    reader.init(length, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(ec == error::http_error_t::need_more);
}

// beast calls finish when the content-length body is complete.
BOOST_AUTO_TEST_CASE(http_body_reader__finish__beast_parsed_content_length_incomplete_rpc__incomplete)
{
    const std::string_view text
    {
        "POST / HTTP/1.1\r\n"
        "Content-Type: application/json\r\n"
        "Content-Length: 7\r\n"
        "\r\n"
        R"({"id":1)"
    };

    const asio::const_buffer buffer{ text.data(), text.size() };
    boost::beast::http::request_parser<body> parser{};
    parser.eager(true);
    boost_code ec{};
    parser.put(buffer, ec);

    BOOST_REQUIRE(ec == error::to_json_code(error::json_error_t::json_incomplete));
}

// beast calls finish when the final chunk is read.
BOOST_AUTO_TEST_CASE(http_body_reader__finish__beast_parsed_chunked_incomplete_rpc__incomplete)
{
    const std::string_view text
    {
        "POST / HTTP/1.1\r\n"
        "Content-Type: application/json\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "7\r\n"
        R"({"id":1)" "\r\n"
        "0\r\n"
        "\r\n"
    };

    const asio::const_buffer buffer{ text.data(), text.size() };
    boost::beast::http::request_parser<body> parser{};
    parser.eager(true);
    boost_code ec{};
    parser.put(buffer, ec);

    BOOST_REQUIRE(ec == error::to_json_code(error::json_error_t::json_incomplete));
}

BOOST_AUTO_TEST_SUITE_END()

///#endif // HAVE_SLOW_TESTS
