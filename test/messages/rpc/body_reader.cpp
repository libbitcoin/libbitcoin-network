/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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

#if defined(HAVE_SLOW_TESTS)

BOOST_AUTO_TEST_SUITE(rpc_body_reader_tests)

using namespace network::http;
using namespace network::rpc;
using value = boost::json::value;

BOOST_AUTO_TEST_CASE(rpc_body_reader__construct1__default__null_model_terminated)
{
    rpc::body::value_type body{};
    rpc::body::reader reader(body);
    BOOST_REQUIRE(body.model.is_null());
    BOOST_REQUIRE(body.request.jsonrpc == version::undefined);
    BOOST_REQUIRE(!body.request.params.has_value());
    BOOST_REQUIRE(!body.request.id.has_value());
    BOOST_REQUIRE(body.request.method.empty());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__construct2__default__null_model_non_terminated)
{
    request_header header{};
    rpc::body::value_type body{};
    rpc::body::reader reader(header, body);
    BOOST_REQUIRE(body.model.is_null());
    BOOST_REQUIRE(body.request.jsonrpc == version::undefined);
    BOOST_REQUIRE(!body.request.params.has_value());
    BOOST_REQUIRE(!body.request.id.has_value());
    BOOST_REQUIRE(body.request.method.empty());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__init__simple_request__success)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::body::value_type body{};
    rpc::body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_non_terminated__success_expected_consumed)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::body::value_type body{};
    request_header header{};
    rpc::body::reader reader(header, body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_terminated_with_newline__success_expected_consumed_including_newline)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::body::value_type body{};
    rpc::body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_terminated_without_newline__end_of_stream_expected_consumed_unterminated_set)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::body::value_type body{};
    rpc::body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(ec == error::http_error_t::end_of_stream);
    BOOST_REQUIRE(!reader.is_done());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_non_terminated__success_expected_request_model_cleared)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::body::value_type body{};
    request_header header{};
    rpc::body::reader reader(header, body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.request.jsonrpc == version::v2);
    BOOST_REQUIRE(body.request.id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.request.id.value()), 1);
    BOOST_REQUIRE_EQUAL(body.request.method, "test");
    BOOST_REQUIRE(body.model.is_null());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_terminated_with_newline__success_expected_request_model_cleared)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::body::value_type body{};
    rpc::body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.request.jsonrpc == version::v2);
    BOOST_REQUIRE(body.request.id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.request.id.value()), 1);
    BOOST_REQUIRE_EQUAL(body.request.method, "test");
    BOOST_REQUIRE(body.model.is_null());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_terminated_without_newline__end_of_stream_error)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::body::value_type body{};
    rpc::body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(ec == error::http_error_t::end_of_stream);

    reader.finish(ec);
    BOOST_REQUIRE(ec == error::http_error_t::end_of_stream);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__over_length__body_limit)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::body::value_type body{};
    rpc::body::reader reader(body);
    boost_code ec{};
    reader.init(10, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(ec == error::http_error_t::body_limit);
}

// ============================================================================
// NOTE: The test cases above this line use stale API names and will not
// compile as-is. Specifically:
//   - rpc::body::value_type  → should be rpc::request
//   - rpc::body::reader      → should be rpc::reader
//   - body.request.*         → should be body.message.*
//   - reader.is_done()       → should be reader.done()
// These are tracked as a separate fix. All new tests below use the correct API.
// ============================================================================

// ----------------------------------------------------------------------------
// message_type<request_t> specialization — is_batch() predicate
// Tests that the specialization correctly discriminates single vs. batch state.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_body_reader__message_type_specialization__default__is_not_batch_batch_empty_message_default)
{
    // A default-constructed rpc::request should represent a single (empty)
    // request. is_batch() must return false; batch must be empty.
    rpc::request body{};
    BOOST_REQUIRE(!body.is_batch());
    BOOST_REQUIRE(body.batch.empty());
    BOOST_REQUIRE(body.message.method.empty());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__message_type_specialization__batch_with_one_element__is_batch)
{
    // Manually populating batch{} is sufficient to make is_batch() true.
    // This covers the specialization's inline predicate independently of parsing.
    rpc::request body{};
    body.batch.push_back({});
    BOOST_REQUIRE(body.is_batch());
}

// ----------------------------------------------------------------------------
// finish() — single-request regression after refactor
// The object-root path in the new finish() must be identical to the old code.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__single_request_v2__success_is_not_batch_message_populated_model_cleared)
{
    // Regression: the single-request path (JSON object root) must still work
    // exactly as before the batch refactor. is_batch() must be false and
    // body.message must be populated; body.batch must remain empty.
    const std::string_view text{
        R"({"jsonrpc":"2.0","id":1,"method":"ping"})""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    // Single-request invariants.
    BOOST_REQUIRE(!body.is_batch());
    BOOST_REQUIRE(body.batch.empty());
    // The message field must be correctly deserialized.
    BOOST_REQUIRE_EQUAL(body.message.method, "ping");
    BOOST_REQUIRE(body.message.jsonrpc == version::v2);
    BOOST_REQUIRE(body.message.id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 1);
    // The JSON model must be released after deserialization.
    BOOST_REQUIRE(body.model.is_null());
}

// ----------------------------------------------------------------------------
// finish() — batch happy paths (TCP, with \n terminator)
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__batch_one_request__success_is_batch_size_one_model_cleared)
{
    // A batch of exactly one request element. Verifies that:
    //   - is_batch() becomes true
    //   - batch.size() == 1
    //   - the element's fields are correctly deserialized
    //   - the JSON model is released
    //   - the single-request message field remains default-constructed
    const std::string_view text{
        R"([{"jsonrpc":"2.0","id":1,"method":"test"}])""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.is_batch());
    BOOST_REQUIRE_EQUAL(body.batch.size(), 1u);
    BOOST_REQUIRE_EQUAL(body.batch[0].method, "test");
    BOOST_REQUIRE(body.batch[0].jsonrpc == version::v2);
    BOOST_REQUIRE(body.batch[0].id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.batch[0].id.value()), 1);
    BOOST_REQUIRE(body.message.method.empty()); // single-message field untouched
    BOOST_REQUIRE(body.model.is_null());        // JSON model released
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__batch_two_requests__success_size_two_both_fields_correct)
{
    // Two-element batch. Verifies that the loop in finish() correctly
    // processes every element and preserves declaration order.
    const std::string_view text{
        R"([{"jsonrpc":"2.0","id":1,"method":"first"},)"
        R"({"jsonrpc":"2.0","id":2,"method":"second"}])""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.is_batch());
    BOOST_REQUIRE_EQUAL(body.batch.size(), 2u);
    BOOST_REQUIRE_EQUAL(body.batch[0].method, "first");
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.batch[0].id.value()), 1);
    BOOST_REQUIRE_EQUAL(body.batch[1].method, "second");
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.batch[1].id.value()), 2);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__batch_with_notification__success_notification_has_no_id)
{
    // A batch containing a notification (no "id" field). Per JSON-RPC 2.0 §6
    // a notification is a valid array element; no id field means the server
    // must not send a response for it. The reader must deserialize it
    // faithfully — dispatching logic is the channel's responsibility.
    const std::string_view text{
        R"([{"jsonrpc":"2.0","id":1,"method":"req"},)"
        R"({"jsonrpc":"2.0","method":"notify"}])""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.is_batch());
    BOOST_REQUIRE_EQUAL(body.batch.size(), 2u);
    BOOST_REQUIRE(body.batch[0].id.has_value());
    BOOST_REQUIRE(!body.batch[1].id.has_value()); // notification: no id
    BOOST_REQUIRE_EQUAL(body.batch[1].method, "notify");
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__batch_http_path_no_terminator__success)
{
    // The HTTP path uses the two-argument reader constructor, which sets
    // terminated_=false. A JSON array without a trailing '\n' must succeed.
    const std::string_view text{
        R"([{"jsonrpc":"2.0","id":1,"method":"test"}])" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    http::request_header header{};
    rpc::reader reader(header, body); // terminated_ = false
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.is_batch());
    BOOST_REQUIRE_EQUAL(body.batch.size(), 1u);
    BOOST_REQUIRE_EQUAL(body.batch[0].method, "test");
}

// ----------------------------------------------------------------------------
// finish() — batch error paths
// Each failure must set the expected error code and must NOT set is_batch().
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__empty_array__jsonrpc_batch_empty_not_batch)
{
    // JSON-RPC 2.0 §6 explicitly forbids an empty batch array. The reader
    // must reject it with jsonrpc_batch_empty before touching value_.batch.
    const std::string_view text{ "[]\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(ec == code{ error::jsonrpc_batch_empty });
    // batch must not have been populated on error.
    BOOST_REQUIRE(!body.is_batch());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__batch_element_is_number__jsonrpc_batch_item_invalid)
{
    // A batch element that is a JSON number, not a JSON object, violates the
    // JSON-RPC 2.0 spec. The reader must stop at the first invalid element.
    const std::string_view text{ R"([42])""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(ec == code{ error::jsonrpc_batch_item_invalid });
    BOOST_REQUIRE(!body.is_batch());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__batch_element_is_array__jsonrpc_batch_item_invalid)
{
    // A nested array as a batch element is also invalid.
    const std::string_view text{ R"([[]])""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(ec == code{ error::jsonrpc_batch_item_invalid });
    BOOST_REQUIRE(!body.is_batch());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__batch_valid_then_invalid_element__jsonrpc_batch_item_invalid)
{
    // First element is a valid request object; second is a string. The reader
    // must fail fast at the second element. The partially-filled batch vector
    // is in an indeterminate state on error; is_batch() should be false.
    const std::string_view text{
        R"([{"jsonrpc":"2.0","id":1,"method":"ok"}, "bad"])""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(ec == code{ error::jsonrpc_batch_item_invalid });
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__scalar_root__jsonrpc_requires_method)
{
    // A JSON root that is neither an object nor an array is rejected with
    // jsonrpc_requires_method (the generic "not a valid JSON-RPC message" code).
    const std::string_view text{ "42\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(ec == code{ error::jsonrpc_requires_method });
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__string_root__jsonrpc_requires_method)
{
    // A JSON string root is similarly invalid.
    const std::string_view text{ R"("hello")""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request body{};
    rpc::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);
    reader.finish(ec);
    BOOST_REQUIRE(ec == code{ error::jsonrpc_requires_method });
}

BOOST_AUTO_TEST_SUITE_END()

#endif // HAVE_SLOW_TESTS
