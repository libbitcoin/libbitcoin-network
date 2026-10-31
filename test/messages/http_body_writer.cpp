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
  : public body::writer
{
    using base = body::writer;
    using base::writer;
    using base::to_writer;
};

BC_AUTO_TEST_SUITE(http_body_writer_tests)

BC_AUTO_TEST_CASE(http_body_writer__to_writer__undefined__constructs_empty_writer)
{
    response_header header{};
    body::value_type value{};
    ///value = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__to_writer__empty__constructs_empty_writer)
{
    response_header header{};
    body::value_type value{};
    value = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__to_writer__json__constructs_json_writer)
{
    response_header header{};
    body::value_type value{};
    value = json_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<json_writer>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__to_writer__data__constructs_data_writer)
{
    response_header header{};
    body::value_type value{};
    value = chunk_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<data_writer>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__to_writer__span__constructs_span_writer)
{
    response_header header{};
    body::value_type value{};
    value = span_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<span_writer>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__to_writer__buffer__constructs_buffer_writer)
{
    response_header header{};
    body::value_type value{};
    value = buffer_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<buffer_writer>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__to_writer__string__constructs_string_writer)
{
    response_header header{};
    body::value_type value{};
    value = string_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<string_writer>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__to_writer__rpc_response__constructs_rpc_writer)
{
    response_header header{};
    body::value_type value{};
    value = rpc::response{};
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<rpc::writer>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__to_writer__rpc_request__constructs_rpc_notifier)
{
    response_header header{};
    body::value_type value{};
    value = rpc::request{};
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<rpc::notifier>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__value_get__const_string__expected)
{
    body::value_type value{};
    value = string_value{ "abc" };
    const auto& constant = value;
    BC_REQUIRE(constant.contains<string_value>());
    BC_REQUIRE_EQUAL(constant.get<string_value>(), "abc");
}

BC_AUTO_TEST_SUITE_END()

BC_FIXTURE_TEST_SUITE(variant_body_writer_file_body_tests, test::directory_setup_fixture)

BC_AUTO_TEST_CASE(http_body_writer__to_writer__file__constructs_file_writer)
{
    // In dubug builds boost asserts that the file is open.
    // BC_ASSERT(body_.file_.is_open());
    code ec{};
    file_body::value_type file{};
    BC_REQUIRE(file.open_write(TEST_PATH));

    response_header header{};
    body::value_type value{};
    value = std::move(file);
    const auto variant = accessor::to_writer(header, value);
    BC_REQUIRE(std::holds_alternative<file_writer>(variant));
}

BC_AUTO_TEST_CASE(http_body_writer__get__file__expected_content_no_more)
{
    const std::string expected{ "abc" };
    std::ofstream{ TEST_PATH } << expected;

    code ec{};
    file_body::value_type file{};
    BC_REQUIRE(file.open_read(TEST_PATH));

    response_header header{};
    body::value_type value{};
    value = std::move(file);
    body::writer writer(header, value);
    BC_REQUIRE(writer.binary());

    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE_EQUAL(std::string(system::pointer_cast<const char>(buffer.value().first.data()), buffer.value().first.size()), expected);
}

BC_AUTO_TEST_SUITE_END()

////#endif // HAVE_SLOW_TESTS

