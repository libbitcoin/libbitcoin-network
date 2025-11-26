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
using namespace network::monad;

struct accessor
  : public body::writer
{
    using base = body::writer;
    using base::writer;
    using base::to_writer;
};

BOOST_AUTO_TEST_SUITE(monad_body_writer_tests)

BOOST_AUTO_TEST_CASE(monad_body_writer__to_writer__undefined__constructs_empty_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    ///value = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BOOST_AUTO_TEST_CASE(monad_body_writer__to_writer__empty__constructs_empty_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BOOST_AUTO_TEST_CASE(monad_body_writer__to_writer__json__constructs_json_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = json_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<json_writer>(variant));
}

BOOST_AUTO_TEST_CASE(monad_body_writer__to_writer__data__constructs_data_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = data_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<data_writer>(variant));
}

BOOST_AUTO_TEST_CASE(monad_body_writer__to_writer__span__constructs_span_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = span_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<span_writer>(variant));
}

BOOST_AUTO_TEST_CASE(monad_body_writer__to_writer__buffer__constructs_buffer_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = buffer_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<buffer_writer>(variant));
}

BOOST_AUTO_TEST_CASE(monad_body_writer__to_writer__string__constructs_string_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = string_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<string_writer>(variant));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(variant_body_writer_file_body_tests, test::directory_setup_fixture)

BOOST_AUTO_TEST_CASE(monad_body_writer__to_writer__file__constructs_file_writer)
{
    // In dubug builds boost asserts that the file is open.
    // BOOST_ASSERT(body_.file_.is_open());
    boost_code ec{};
    file_body::value_type file{};
    file.open((TEST_PATH).c_str(), boost::beast::file_mode::write, ec);
    BOOST_REQUIRE(!ec);

    message_header<false, fields> header{};
    body::value_type value{};
    value = std::move(file);
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<file_writer>(variant));
}

BOOST_AUTO_TEST_SUITE_END()
