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
        body::value_type& value) NOEXCEPT
      : base(header, value)
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

// TODO: linux debug boost asserts on body_.file_.is_open().
////BOOST_AUTO_TEST_CASE(http_body_reader__init__default_body_application_octet_stream_with_attachment__constructs_file_reader)
////{
////    message_header<true, fields> header{};
////    header.set(http::field::content_type, "application/octet-stream");
////    header.set(http::field::content_disposition, "filename=somenonsense.jpg");
////    body::value_type value{};
////    accessor reader(header, value);
////    length_type length{ max_size_t };
////    boost_code ec{};
////    reader.init(length, ec);
////    BOOST_REQUIRE(std::holds_alternative<file_reader>(reader.reader_));
////}

// TODO: linux debug boost asserts on body_.file_.is_open().
////BOOST_AUTO_TEST_CASE(http_body_reader__init__default_body_application_octet_stream_with_dirty_attachment__constructs_file_reader)
////{
////    message_header<true, fields> header{};
////    header.set(http::field::content_type, "application/octet-stream");
////    header.set(http::field::content_disposition, "dirty 42; filename* = somenonsense.jpg; some other nonsense");
////    body::value_type value{};
////    accessor reader(header, value);
////    length_type length{ max_size_t };
////    boost_code ec{};
////    reader.init(length, ec);
////    BOOST_REQUIRE(std::holds_alternative<file_reader>(reader.reader_));
////}

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

BOOST_AUTO_TEST_SUITE_END()

///#endif // HAVE_SLOW_TESTS
