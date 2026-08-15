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

BOOST_AUTO_TEST_SUITE(http_body_writer_tests)

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__undefined__constructs_empty_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    ///value = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__empty__constructs_empty_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__json__constructs_json_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = json_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<json_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__data__constructs_data_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = chunk_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<data_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__span__constructs_span_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = span_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<span_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__buffer__constructs_buffer_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = buffer_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<buffer_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__string__constructs_string_writer)
{
    message_header<false, fields> header{};
    body::value_type value{};
    value = string_body::value_type{};
    const auto variant = accessor::to_writer(header, value);
    BOOST_REQUIRE(std::holds_alternative<string_writer>(variant));
}

// size
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(http_body__size__unassigned__zero)
{
    const body::value_type value{};
    BOOST_REQUIRE_EQUAL(body::size(value), 0u);
}

BOOST_AUTO_TEST_CASE(http_body__size__empty__zero)
{
    body::value_type value{};
    value = empty_body::value_type{};
    BOOST_REQUIRE_EQUAL(body::size(value), 0u);
}

BOOST_AUTO_TEST_CASE(http_body__size__data__data_length)
{
    const chunk_body::value_type expected{ 0x2a, 0x2b, 0x2c };
    body::value_type value{};
    value = chunk_body::value_type{ expected };
    BOOST_REQUIRE_EQUAL(body::size(value), expected.size());
}

// The span body requires a non-const pointer (see beast.hpp).
BOOST_AUTO_TEST_CASE(http_body__size__span__span_length)
{
    std::vector<uint8_t> expected{ 0x2a, 0x2b, 0x2c };
    body::value_type value{};
    value = span_body::value_type{ expected.data(), expected.size() };
    BOOST_REQUIRE_EQUAL(body::size(value), expected.size());
}

// The buffer body requires a non-const pointer (see beast.hpp).
BOOST_AUTO_TEST_CASE(http_body__size__buffer__buffer_length)
{
    std::vector<uint8_t> expected{ 0x2a, 0x2b, 0x2c };
    buffer_body::value_type buffer{};
    buffer.data = expected.data();
    buffer.size = expected.size();

    body::value_type value{};
    value = std::move(buffer);
    BOOST_REQUIRE_EQUAL(body::size(value), expected.size());
}

// beast emits no bytes for an unassigned buffer, whatever size is assigned.
BOOST_AUTO_TEST_CASE(http_body__size__buffer_unassigned_data__zero)
{
    const std::vector<uint8_t> unassigned{ 0x2a, 0x2b, 0x2c };
    buffer_body::value_type buffer{};
    buffer.size = unassigned.size();

    body::value_type value{};
    value = std::move(buffer);
    BOOST_REQUIRE_EQUAL(body::size(value), 0u);
}

BOOST_AUTO_TEST_CASE(http_body__size__string__string_length)
{
    const std::string_view expected{ "hello" };
    body::value_type value{};
    value = string_body::value_type{ expected };
    BOOST_REQUIRE_EQUAL(body::size(value), expected.size());
}

BOOST_AUTO_TEST_CASE(http_body__size__json__serialized_model_length)
{
    const std::string_view expected{ R"({"result":42})" };
    json_body::value_type json{};
    json.model = boost::json::parse(expected);

    body::value_type value{};
    value = std::move(json);
    BOOST_REQUIRE_EQUAL(body::size(value), expected.size());
}

BOOST_AUTO_TEST_CASE(http_body__size__rpc_request__message_length)
{
    const std::string_view expected{ R"({"jsonrpc":"2.0","method":"notify"})" };
    rpc::request request{};
    request.message = rpc::request_t{ rpc::version::v2, {}, "notify", {} };

    body::value_type value{};
    value = std::move(request);
    BOOST_REQUIRE_EQUAL(body::size(value), expected.size());
}

// The http response carries no terminator, so the content_length is exactly
// the serialized message (a stream message adds a newline terminator).
BOOST_AUTO_TEST_CASE(http_body__size__rpc_response__message_length)
{
    const std::string_view expected{ R"({"jsonrpc":"2.0","id":1,"result":true})" };
    rpc::response response{};
    response.message = rpc::response_t{ rpc::version::v2, rpc::identity_t{ 1 }, {}, rpc::value_t{ true } };

    body::value_type value{};
    value = std::move(response);
    BOOST_REQUIRE_EQUAL(body::size(value), expected.size());
}

// unframable_zero
// ----------------------------------------------------------------------------
// A model serializes to at least a null, and a peer body is never framed, so
// a zero measure of either is not a length the writer will honor.

BOOST_AUTO_TEST_CASE(http_body__unframable_zero__unassigned__false)
{
    const body::value_type value{};
    BOOST_REQUIRE(!body::unframable_zero(value));
}

BOOST_AUTO_TEST_CASE(http_body__unframable_zero__string__false)
{
    body::value_type value{};
    value = string_value{ "42" };
    BOOST_REQUIRE(!body::unframable_zero(value));
}

BOOST_AUTO_TEST_CASE(http_body__unframable_zero__empty__false)
{
    body::value_type value{};
    value = empty_value{};
    BOOST_REQUIRE(!body::unframable_zero(value));
}

BOOST_AUTO_TEST_CASE(http_body__unframable_zero__json__true)
{
    json_body::value_type json{};
    json.model = boost::json::parse(R"({"result":42})");

    body::value_type value{};
    value = std::move(json);
    BOOST_REQUIRE(body::unframable_zero(value));
}

BOOST_AUTO_TEST_CASE(http_body__unframable_zero__rpc_response__true)
{
    rpc::response response{};
    response.message = rpc::response_t{ rpc::version::v2, rpc::identity_t{ 1 }, {}, rpc::value_t{ true } };

    body::value_type value{};
    value = std::move(response);
    BOOST_REQUIRE(body::unframable_zero(value));
}

BOOST_AUTO_TEST_CASE(http_body__unframable_zero__peer__true)
{
    body::value_type value{};
    value = peer_value{};
    BOOST_REQUIRE(body::unframable_zero(value));
}

BOOST_AUTO_TEST_CASE(http_body__unframable_zero__rpc_request__true)
{
    rpc::request request{};
    request.message = rpc::request_t{ rpc::version::v2, rpc::identity_t{ 1 }, "method", {} };

    body::value_type value{};
    value = std::move(request);
    BOOST_REQUIRE(body::unframable_zero(value));
}

// A null model is four bytes, so no serializable model measures zero.
BOOST_AUTO_TEST_CASE(http_body__size__json_null_model__four)
{
    json_body::value_type json{};
    json.model = boost::json::value{};

    body::value_type value{};
    value = std::move(json);
    BOOST_REQUIRE_EQUAL(body::size(value), 4u);
}

// streaming
// ----------------------------------------------------------------------------
// A streaming body cannot be framed with a content_length, so the sender
// refuses to write one that the caller has not set chunked.

BOOST_AUTO_TEST_CASE(http_body__streaming__unassigned__false)
{
    const body::value_type value{};
    BOOST_REQUIRE(!body::streaming(value));
}

BOOST_AUTO_TEST_CASE(http_body__streaming__string__false)
{
    body::value_type value{};
    value = string_body::value_type{ "hello" };
    BOOST_REQUIRE(!body::streaming(value));
}

BOOST_AUTO_TEST_CASE(http_body__streaming__buffer_last__false)
{
    std::vector<uint8_t> expected{ 0x2a, 0x2b, 0x2c };
    buffer_body::value_type buffer{};
    buffer.data = expected.data();
    buffer.size = expected.size();
    buffer.more = false;

    body::value_type value{};
    value = std::move(buffer);
    BOOST_REQUIRE(!body::streaming(value));
}

BOOST_AUTO_TEST_CASE(http_body__streaming__buffer_more__true)
{
    std::vector<uint8_t> expected{ 0x2a, 0x2b, 0x2c };
    buffer_body::value_type buffer{};
    buffer.data = expected.data();
    buffer.size = expected.size();
    buffer.more = true;

    body::value_type value{};
    value = std::move(buffer);
    BOOST_REQUIRE(body::streaming(value));
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_FIXTURE_TEST_SUITE(variant_body_writer_file_body_tests, test::directory_setup_fixture)

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__file__constructs_file_writer)
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

////#endif // HAVE_SLOW_TESTS

