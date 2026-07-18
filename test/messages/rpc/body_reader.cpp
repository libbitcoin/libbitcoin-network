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

////#if defined(HAVE_SLOW_TESTS)

BOOST_AUTO_TEST_SUITE(rpc_body_reader_tests)

using namespace network::http;
using namespace network::rpc;
using value = boost::json::value;

BOOST_AUTO_TEST_CASE(rpc_body_reader__construct1__default__null_model_terminated)
{
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    BOOST_REQUIRE(body.model.is_null());
    BOOST_REQUIRE(body.message.jsonrpc == version::undefined);
    BOOST_REQUIRE(!body.message.params.has_value());
    BOOST_REQUIRE(!body.message.id.has_value());
    BOOST_REQUIRE(body.message.method.empty());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__construct2__default__null_model_non_terminated)
{
    request_header header{};
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(header, body);
    BOOST_REQUIRE(body.model.is_null());
    BOOST_REQUIRE(body.message.jsonrpc == version::undefined);
    BOOST_REQUIRE(!body.message.params.has_value());
    BOOST_REQUIRE(!body.message.id.has_value());
    BOOST_REQUIRE(body.message.method.empty());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__init__simple_request__success)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_non_terminated__success_expected_consumed)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
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
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_terminated_without_newline__consumed_not_done)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!reader.done());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_non_terminated__success_expected_request_model_cleared)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.message.jsonrpc == version::v2);
    BOOST_REQUIRE(body.message.id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 1);
    BOOST_REQUIRE_EQUAL(body.message.method, "test");
    BOOST_REQUIRE(body.model.is_null());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_terminated_with_newline__success_expected_request_model_cleared)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})""\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.message.jsonrpc == version::v2);
    BOOST_REQUIRE(body.message.id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 1);
    BOOST_REQUIRE_EQUAL(body.message.method, "test");
    BOOST_REQUIRE(body.model.is_null());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_terminated_without_newline__need_more_not_done)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(!ec);

    reader.finish(ec);
    BOOST_REQUIRE(ec == error::http_error_t::need_more);
    BOOST_REQUIRE(!reader.done());
}

// batch
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__batch_open_with_first_element__success_changed_separator_unconsumed)
{
    const std::string_view text{ R"([{"jsonrpc":"2.0","id":1,"method":"test"},)" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    // Trailing separator is not consumed (next read prologue).
    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), sub1(text.size()));
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(reader.done());
    BOOST_REQUIRE(body.changed);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.message.jsonrpc == version::v2);
    BOOST_REQUIRE(body.message.id.has_value());
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 1);
    BOOST_REQUIRE_EQUAL(body.message.method, "test");
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__batch_element_with_separator__success_unchanged)
{
    const std::string_view text{ R"(,{"jsonrpc":"2.0","id":2,"method":"next"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    body.batch = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(reader.done());
    BOOST_REQUIRE(!body.changed);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.message.jsonrpc == version::v2);
    BOOST_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 2);
    BOOST_REQUIRE_EQUAL(body.message.method, "next");
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__batch_close_terminated__success_changed_no_message)
{
    const std::string_view text{ "]\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    body.batch = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(reader.done());
    BOOST_REQUIRE(body.changed);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body.message.method.empty());
    BOOST_REQUIRE(!body.message.id.has_value());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__batch_close_padded_terminator__success)
{
    const std::string_view text{ "] \t\r\n" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    body.batch = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(reader.done());
    BOOST_REQUIRE(body.changed);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__batch_close_split_terminator__need_more_then_done)
{
    const std::string_view close{ "]" };
    const std::string_view line{ "\n" };
    const asio::const_buffer buffer1{ close.data(), close.size() };
    const asio::const_buffer buffer2{ line.data(), line.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    body.batch = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init({}, ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer1, ec), one);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!reader.done());

    reader.finish(ec);
    BOOST_REQUIRE(ec == error::http_error_t::need_more);

    BOOST_REQUIRE_EQUAL(reader.put(buffer2, ec), one);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(reader.done());
    BOOST_REQUIRE(body.changed);

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__empty_batch__batch_empty)
{
    const std::string_view text{ "[ ]" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(ec == error::jsonrpc_batch_empty);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__nested_open__batch_malformed)
{
    const std::string_view text{ "[[" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__reopen_within_batch__batch_malformed)
{
    const std::string_view text{ ",[" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    body.batch = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__separator_outside_batch__batch_malformed)
{
    const std::string_view text{ "," };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__close_outside_batch__batch_malformed)
{
    const std::string_view text{ "]" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__double_separator__batch_malformed)
{
    const std::string_view text{ ",," };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    body.batch = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__close_after_separator__batch_malformed)
{
    const std::string_view text{ ",]" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    body.batch = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__batch_element_without_separator__batch_malformed)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":2,"method":"next"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batchable = true;
    body.batch = true;
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer, ec);
    BOOST_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__full_batch__three_reads_consume_buffer)
{
    // One wire buffer, three logical reads (open+first, second, close).
    const std::string_view first{ R"([{"jsonrpc":"2.0","id":1,"method":"one"})" };
    const std::string_view second{ R"(,{"jsonrpc":"2.0","id":2,"method":"two"})" };
    const std::string_view third{ "]  \n" };
    const std::string text
    {
        R"([{"jsonrpc":"2.0","id":1,"method":"one"},)"
        R"({"jsonrpc":"2.0","id":2,"method":"two"}]  )" "\n"
    };

    // Each read is offered the buffer remainder, consuming one segment.
    const auto offset2 = first.size();
    const auto offset3 = offset2 + second.size();
    const asio::const_buffer buffer1{ &text[zero], text.size() };
    const asio::const_buffer buffer2{ &text[offset2], text.size() - offset2 };
    const asio::const_buffer buffer3{ &text[offset3], text.size() - offset3 };
    boost_code ec{};

    // Read 1: batch open rides along with first element.
    rpc::request_body::value_type body1{};
    body1.batchable = true;
    rpc::request_body::reader reader1(body1);
    reader1.init({}, ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader1.put(buffer1, ec), first.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(reader1.done());
    BOOST_REQUIRE(body1.changed);

    reader1.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(body1.message.method, "one");

    // Read 2: second element (separator consumed in prologue).
    rpc::request_body::value_type body2{};
    body2.batchable = true;
    body2.batch = true;
    rpc::request_body::reader reader2(body2);
    reader2.init({}, ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader2.put(buffer2, ec), second.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(reader2.done());
    BOOST_REQUIRE(!body2.changed);

    reader2.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(body2.message.method, "two");

    // Read 3: batch close (padded terminator), no message.
    rpc::request_body::value_type body3{};
    body3.batchable = true;
    body3.batch = true;
    rpc::request_body::reader reader3(body3);
    reader3.init({}, ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader3.put(buffer3, ec), third.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(reader3.done());
    BOOST_REQUIRE(body3.changed);

    reader3.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body3.message.method.empty());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__full_batch_byte_at_a_time__three_reads_consume_buffer)
{
    // Same batch with every read split at every byte boundary (whitespace
    // padded delimiters). Each segment is one logical read's exact bytes.
    const std::string_view first{ R"( [ {"jsonrpc":"2.0","id":1,"method":"one"})" };
    const std::string_view second{ R"( , {"jsonrpc":"2.0","id":2,"method":"two"})" };
    const std::string_view third{ " ] \n" };
    boost_code ec{};

    // Read 1: batch open rides along with first element.
    rpc::request_body::value_type body1{};
    body1.batchable = true;
    rpc::request_body::reader reader1(body1);
    reader1.init({}, ec);
    BOOST_REQUIRE(!ec);

    for (const auto& byte: first)
    {
        const asio::const_buffer buffer{ &byte, one };
        BOOST_REQUIRE_EQUAL(reader1.put(buffer, ec), one);
        BOOST_REQUIRE(!ec);
    }

    BOOST_REQUIRE(reader1.done());
    BOOST_REQUIRE(body1.changed);

    reader1.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(body1.message.method, "one");

    // Read 2: second element (separator consumed in prologue).
    rpc::request_body::value_type body2{};
    body2.batchable = true;
    body2.batch = true;
    rpc::request_body::reader reader2(body2);
    reader2.init({}, ec);
    BOOST_REQUIRE(!ec);

    for (const auto& byte: second)
    {
        const asio::const_buffer buffer{ &byte, one };
        BOOST_REQUIRE_EQUAL(reader2.put(buffer, ec), one);
        BOOST_REQUIRE(!ec);
    }

    BOOST_REQUIRE(reader2.done());
    BOOST_REQUIRE(!body2.changed);

    reader2.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(body2.message.method, "two");

    // Read 3: batch close (padded terminator), no message.
    rpc::request_body::value_type body3{};
    body3.batchable = true;
    body3.batch = true;
    rpc::request_body::reader reader3(body3);
    reader3.init({}, ec);
    BOOST_REQUIRE(!ec);

    for (const auto& byte: third)
    {
        const asio::const_buffer buffer{ &byte, one };
        BOOST_REQUIRE_EQUAL(reader3.put(buffer, ec), one);
        BOOST_REQUIRE(!ec);
    }

    BOOST_REQUIRE(reader3.done());
    BOOST_REQUIRE(body3.changed);

    reader3.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(body3.message.method.empty());
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__split_padded_terminator_after_singleton__success)
{
    // Terminator split from document across reads, with padding (electrum
    // traffic shaping), previously unsupported.
    const std::string_view document{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const std::string_view padding{ "  \n" };
    const asio::const_buffer buffer1{ document.data(), document.size() };
    const asio::const_buffer buffer2{ padding.data(), padding.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init({}, ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer1, ec), document.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!reader.done());

    reader.finish(ec);
    BOOST_REQUIRE(ec == error::http_error_t::need_more);

    BOOST_REQUIRE_EQUAL(reader.put(buffer2, ec), padding.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(reader.done());

    reader.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(body.message.method, "test");
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__garbage_where_terminator_expected__reader_stall)
{
    const std::string_view document{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const std::string_view garbage{ "x" };
    const asio::const_buffer buffer1{ document.data(), document.size() };
    const asio::const_buffer buffer2{ garbage.data(), garbage.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init({}, ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer1, ec);
    BOOST_REQUIRE(!ec);

    reader.put(buffer2, ec);
    BOOST_REQUIRE(ec == error::jsonrpc_reader_stall);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__batch_not_batchable__parsed_as_document_error)
{
    // Without batchable the array parses as a single document, which then
    // fails conversion to a request (current http behavior unchanged).
    const std::string_view text{ R"([{"jsonrpc":"2.0","id":1,"method":"test"}])" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    boost_code ec{};
    reader.init(text.size(), ec);
    BOOST_REQUIRE(!ec);

    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!body.changed);

    reader.finish(ec);
    BOOST_REQUIRE(ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_reader__put__over_length__body_limit)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const asio::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    boost_code ec{};
    reader.init(10, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BOOST_REQUIRE(ec == error::http_error_t::body_limit);
}

BOOST_AUTO_TEST_SUITE_END()

////#endif // HAVE_SLOW_TESTS
