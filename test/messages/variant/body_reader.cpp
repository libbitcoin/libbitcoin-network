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

using namespace http;
using namespace network::variant;

struct accessor
  : public body::reader
{
    using base = body::reader;
    using base::reader;
    using base::to_reader;
};

BOOST_AUTO_TEST_SUITE(variant_body_reader_tests)

BOOST_AUTO_TEST_CASE(variant_body_reader__to_reader__bogus__constructs_empty_reader)
{
    header<false, fields> header{};
    header.set(http::field::content_type, "bogus");
    body::value_type value{};
    value = empty_body::value_type{};
    const auto variant = accessor::to_reader(header, value);
    BOOST_REQUIRE(std::holds_alternative<empty_reader>(variant));
}

BOOST_AUTO_TEST_CASE(variant_body_reader__to_reader__json__constructs_json_reader)
{
    header<false, fields> header{};
    header.set(http::field::content_type, "application/json");
    body::value_type value{};
    value = json_body::value_type{};
    const auto variant = accessor::to_reader(header, value);
    BOOST_REQUIRE(std::holds_alternative<json_reader>(variant));
}

BOOST_AUTO_TEST_CASE(variant_body_reader__to_reader__application_octet_stream__constructs_data_reader)
{
    header<false, fields> header{};
    header.set(http::field::content_type, "application/octet-stream");
    header.set(http::field::content_disposition, "bogus");
    body::value_type value{};
    value = data_body::value_type{};
    const auto variant = accessor::to_reader(header, value);
    BOOST_REQUIRE(std::holds_alternative<data_reader>(variant));
}

BOOST_AUTO_TEST_CASE(variant_body_reader__to_reader__application_octet_stream_with_attachment__constructs_file_reader)
{
    header<false, fields> header{};
    header.set(http::field::content_type, "application/octet-stream");
    header.set(http::field::content_disposition, "filename=somenonsense.jpg");
    body::value_type value{};
    value = file_body::value_type{};
    const auto variant = accessor::to_reader(header, value);
    BOOST_REQUIRE(std::holds_alternative<file_reader>(variant));
}

BOOST_AUTO_TEST_CASE(variant_body_reader__to_reader__application_octet_stream_with_dirty_attachment__constructs_file_reader)
{
    header<false, fields> header{};
    header.set(http::field::content_type, "application/octet-stream");
    header.set(http::field::content_disposition, "dirty 42; filename* = somenonsense.jpg; some other nonsense");
    body::value_type value{};
    value = file_body::value_type{};
    const auto variant = accessor::to_reader(header, value);
    BOOST_REQUIRE(std::holds_alternative<file_reader>(variant));
}

BOOST_AUTO_TEST_CASE(variant_body_reader__to_reader__text_plain__constructs_string_reader)
{
    header<false, fields> header{};
    header.set(http::field::content_type, "text/plain");
    body::value_type value{};
    value = string_body::value_type{};
    const auto variant = accessor::to_reader(header, value);
    BOOST_REQUIRE(std::holds_alternative<string_reader>(variant));
}

BOOST_AUTO_TEST_SUITE_END()
