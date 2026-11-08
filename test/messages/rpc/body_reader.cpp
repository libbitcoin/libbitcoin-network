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
#include "../../test.hpp"

////#if defined(HAVE_SLOW_TESTS)

BC_AUTO_TEST_SUITE(rpc_body_reader_tests)

// boolean_t defined in global namespace by:
// Applications/Xcode_16.4.app/Contents/Developer/Platforms/MacOSX.platform/
// Developer/SDKs/MacOSX.sdk/usr/include/mach/arm/boolean.h:70:25
using boolean_type = bc::network::rpc::boolean_t;

using namespace network::http;
using namespace network::rpc;
using value = system::json::value;

BC_AUTO_TEST_CASE(rpc_body_reader__construct1__default__null_model_terminated)
{
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    BC_REQUIRE(body.model.is_null());
    BC_REQUIRE(body.message.jsonrpc == version::undefined);
    BC_REQUIRE(!body.message.params.has_value());
    BC_REQUIRE(!body.message.id.has_value());
    BC_REQUIRE(body.message.method.empty());
}

BC_AUTO_TEST_CASE(rpc_body_reader__construct2__default__null_model_non_terminated)
{
    request_header header{};
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(header, body);
    BC_REQUIRE(body.model.is_null());
    BC_REQUIRE(body.message.jsonrpc == version::undefined);
    BC_REQUIRE(!body.message.params.has_value());
    BC_REQUIRE(!body.message.id.has_value());
    BC_REQUIRE(body.message.method.empty());
}

BC_AUTO_TEST_CASE(rpc_body_reader__init__simple_request__success)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_non_terminated__success_expected_consumed)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_terminated_with_newline__success_expected_consumed_including_newline)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})""\n" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__simple_request_terminated_without_newline__consumed_not_done)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(!reader.done());
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_non_terminated__success_expected_request_model_cleared)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(body.message.jsonrpc == version::v2);
    BC_REQUIRE(body.message.id.has_value());
    BC_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 1);
    BC_REQUIRE_EQUAL(body.message.method, "test");
    BC_REQUIRE(body.model.is_null());
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_terminated_with_newline__success_expected_request_model_cleared)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})""\n" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(body.message.jsonrpc == version::v2);
    BC_REQUIRE(body.message.id.has_value());
    BC_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 1);
    BC_REQUIRE_EQUAL(body.message.method, "test");
    BC_REQUIRE(body.model.is_null());
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__simple_request_terminated_without_newline__need_more_not_done)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec == error::http_error_t::need_more);
    BC_REQUIRE(!reader.done());
}

// batch
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(rpc_body_reader__put__batch_open_with_first_element__success_changed_separator_unconsumed)
{
    const std::string_view text{ R"([{"jsonrpc":"2.0","id":1,"method":"test"},)" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    // Trailing separator is not consumed (next read prologue). Delivery
    // pauses the parse (need_buffer) with the converted message.
    BC_REQUIRE_EQUAL(reader.put(buffer, ec), sub1(text.size()));
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body.changed);
    BC_REQUIRE(!body.lax_batch);
    BC_REQUIRE(body.message.jsonrpc == version::v2);
    BC_REQUIRE(body.message.id.has_value());
    BC_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 1);
    BC_REQUIRE_EQUAL(body.message.method, "test");
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__batch_element_with_separator__success_unchanged)
{
    const std::string_view text{ R"(,{"jsonrpc":"2.0","id":2,"method":"next"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(!body.changed);
    BC_REQUIRE(body.message.jsonrpc == version::v2);
    BC_REQUIRE_EQUAL(std::get<code_t>(body.message.id.value()), 2);
    BC_REQUIRE_EQUAL(body.message.method, "next");
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__batch_close_terminated__success_changed_no_message)
{
    const std::string_view text{ "]\n" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body.changed);
    BC_REQUIRE(body.message.method.empty());
    BC_REQUIRE(!body.message.id.has_value());

    reader.finish(ec);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__batch_close_padded_terminator__success)
{
    const std::string_view text{ "] \t\r\n" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body.changed);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__batch_close_split_terminator__need_more_then_done)
{
    const std::string_view close{ "]" };
    const std::string_view line{ "\n" };
    const io::const_buffer buffer1{ close.data(), close.size() };
    const io::const_buffer buffer2{ line.data(), line.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer1, ec), one);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!reader.done());

    reader.finish(ec);
    BC_REQUIRE(ec == error::http_error_t::need_more);

    BC_REQUIRE_EQUAL(reader.put(buffer2, ec), one);
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body.changed);

    reader.finish(ec);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__empty_batch__batch_empty)
{
    const std::string_view text{ "[ ]" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(ec == error::jsonrpc_batch_empty);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__nested_open__batch_malformed)
{
    const std::string_view text{ "[[" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__reopen_within_batch__batch_malformed)
{
    const std::string_view text{ ",[" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__separator_outside_batch__batch_malformed)
{
    const std::string_view text{ "," };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__close_outside_batch__batch_malformed)
{
    const std::string_view text{ "]" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__double_separator__batch_malformed)
{
    const std::string_view text{ ",," };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__close_after_separator__batch_malformed)
{
    const std::string_view text{ ",]" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__batch_element_without_separator__batch_malformed)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":2,"method":"next"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__full_batch__three_reads_consume_buffer)
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
    const io::const_buffer buffer1{ &text[zero], text.size() };
    const io::const_buffer buffer2{ &text[offset2], text.size() - offset2 };
    const io::const_buffer buffer3{ &text[offset3], text.size() - offset3 };
    code ec{};

    // Read 1: batch open rides along with first element.
    rpc::request_body::value_type body1{};
    rpc::request_body::reader reader1(body1);
    reader1.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader1.put(buffer1, ec), first.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body1.changed);
    BC_REQUIRE_EQUAL(body1.message.method, "one");

    // Read 2: second element (separator consumed in prologue).
    rpc::request_body::value_type body2{};
    body2.batch = true;
    rpc::request_body::reader reader2(body2);
    reader2.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader2.put(buffer2, ec), second.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(!body2.changed);
    BC_REQUIRE_EQUAL(body2.message.method, "two");

    // Read 3: batch close (padded terminator), no message.
    rpc::request_body::value_type body3{};
    body3.batch = true;
    rpc::request_body::reader reader3(body3);
    reader3.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader3.put(buffer3, ec), third.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body3.changed);
    BC_REQUIRE(body3.message.method.empty());

    reader3.finish(ec);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__full_batch_byte_at_a_time__three_reads_consume_buffer)
{
    // Same batch with every read split at every byte boundary (whitespace
    // padded delimiters). Each segment is one logical read's exact bytes.
    const std::string_view first{ R"( [ {"jsonrpc":"2.0","id":1,"method":"one"})" };
    const std::string_view second{ R"( , {"jsonrpc":"2.0","id":2,"method":"two"})" };
    const std::string_view third{ " ] \n" };
    code ec{};

    // Read 1: batch open rides along with first element.
    rpc::request_body::value_type body1{};
    rpc::request_body::reader reader1(body1);
    reader1.init({}, ec);
    BC_REQUIRE(!ec);

    for (const auto& byte: first.substr(zero, sub1(first.size())))
    {
        const io::const_buffer buffer{ &byte, one };
        BC_REQUIRE_EQUAL(reader1.put(buffer, ec), one);
        BC_REQUIRE(!ec);
    }

    // The final element byte delivers the message (need_buffer).
    const io::const_buffer last1{ &first.back(), one };
    BC_REQUIRE_EQUAL(reader1.put(last1, ec), one);
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body1.changed);
    BC_REQUIRE_EQUAL(body1.message.method, "one");

    // Read 2: second element (separator consumed in prologue).
    rpc::request_body::value_type body2{};
    body2.batch = true;
    rpc::request_body::reader reader2(body2);
    reader2.init({}, ec);
    BC_REQUIRE(!ec);

    for (const auto& byte: second.substr(zero, sub1(second.size())))
    {
        const io::const_buffer buffer{ &byte, one };
        BC_REQUIRE_EQUAL(reader2.put(buffer, ec), one);
        BC_REQUIRE(!ec);
    }

    const io::const_buffer last2{ &second.back(), one };
    BC_REQUIRE_EQUAL(reader2.put(last2, ec), one);
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(!body2.changed);
    BC_REQUIRE_EQUAL(body2.message.method, "two");

    // Read 3: batch close (padded terminator), no message.
    rpc::request_body::value_type body3{};
    body3.batch = true;
    rpc::request_body::reader reader3(body3);
    reader3.init({}, ec);
    BC_REQUIRE(!ec);

    for (const auto& byte: third.substr(zero, sub1(third.size())))
    {
        const io::const_buffer buffer{ &byte, one };
        BC_REQUIRE_EQUAL(reader3.put(buffer, ec), one);
        BC_REQUIRE(!ec);
    }

    // The terminator byte delivers the close (need_buffer), no message.
    const io::const_buffer last3{ &third.back(), one };
    BC_REQUIRE_EQUAL(reader3.put(last3, ec), one);
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body3.changed);
    BC_REQUIRE(body3.message.method.empty());

    reader3.finish(ec);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__full_batch_single_reader__delivers_each_element_and_close)
{
    // One reader instance spans the whole message (http parse), with
    // in-batch context carried by the instance rather than caller state.
    const std::string_view first{ R"([{"jsonrpc":"2.0","id":1,"method":"one"})" };
    const std::string_view second{ R"(,{"jsonrpc":"2.0","id":2,"method":"two"})" };
    const std::string_view third{ "]" };
    const std::string_view pad{ " " };
    const std::string text
    {
        R"([{"jsonrpc":"2.0","id":1,"method":"one"},)"
        R"({"jsonrpc":"2.0","id":2,"method":"two"}] )"
    };

    // Each put is offered the remainder, consuming one delivery.
    const auto offset2 = first.size();
    const auto offset3 = offset2 + second.size();
    const auto offset4 = offset3 + third.size();
    const io::const_buffer buffer1{ &text[zero], text.size() };
    const io::const_buffer buffer2{ &text[offset2], text.size() - offset2 };
    const io::const_buffer buffer3{ &text[offset3], text.size() - offset3 };
    const io::const_buffer buffer4{ &text[offset4], text.size() - offset4 };
    request_header header{};
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer1, ec), first.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body.changed);
    BC_REQUIRE_EQUAL(body.message.method, "one");

    body.changed = false;
    BC_REQUIRE_EQUAL(reader.put(buffer2, ec), second.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(!body.changed);
    BC_REQUIRE_EQUAL(body.message.method, "two");

    BC_REQUIRE_EQUAL(reader.put(buffer3, ec), third.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body.changed);

    // Trailing body whitespace is consumed following the close.
    BC_REQUIRE_EQUAL(reader.put(buffer4, ec), pad.size());
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__truncated_batch_single_reader__need_more)
{
    // Message ends (finish) with the batch open and unclosed.
    const std::string_view text{ R"([{"jsonrpc":"2.0","id":1,"method":"one"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    request_header header{};
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body.changed);
    BC_REQUIRE_EQUAL(body.message.method, "one");

    reader.finish(ec);
    BC_REQUIRE(ec == error::http_error_t::need_more);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__split_padded_terminator_after_singleton__success)
{
    // Terminator split from document across reads, with padding (electrum
    // traffic shaping), previously unsupported.
    const std::string_view document{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const std::string_view padding{ "  \n" };
    const io::const_buffer buffer1{ document.data(), document.size() };
    const io::const_buffer buffer2{ padding.data(), padding.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer1, ec), document.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(!reader.done());

    reader.finish(ec);
    BC_REQUIRE(ec == error::http_error_t::need_more);

    BC_REQUIRE_EQUAL(reader.put(buffer2, ec), padding.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());

    reader.finish(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(body.message.method, "test");
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__garbage_where_terminator_expected__reader_stall)
{
    const std::string_view document{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const std::string_view garbage{ "x" };
    const io::const_buffer buffer1{ document.data(), document.size() };
    const io::const_buffer buffer2{ garbage.data(), garbage.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init({}, ec);
    BC_REQUIRE(!ec);

    reader.put(buffer1, ec);
    BC_REQUIRE(!ec);

    reader.put(buffer2, ec);
    BC_REQUIRE(ec == error::jsonrpc_reader_stall);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__split_whitespace_after_non_terminated_singleton__consumed)
{
    const std::string_view document{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const std::string_view whitespace{ "\r\n" };
    const io::const_buffer buffer1{ document.data(), document.size() };
    const io::const_buffer buffer2{ whitespace.data(), whitespace.size() };
    request_header header{};
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer1, ec), document.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());

    BC_REQUIRE_EQUAL(reader.put(buffer2, ec), whitespace.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());

    reader.finish(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(body.message.method, "test");
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__split_garbage_after_non_terminated_singleton__reader_stall)
{
    const std::string_view document{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const std::string_view garbage{ " x" };
    const io::const_buffer buffer1{ document.data(), document.size() };
    const io::const_buffer buffer2{ garbage.data(), garbage.size() };
    request_header header{};
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer1, ec), document.size());
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer2, ec), 1u);
    BC_REQUIRE(ec == error::jsonrpc_reader_stall);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__garbage_within_non_terminated_singleton__reader_stall)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"}x)" };
    const auto document = text.substr(0, sub1(text.size()));
    const auto garbage = text.substr(document.size());
    const io::const_buffer buffer1{ text.data(), text.size() };
    const io::const_buffer buffer2{ garbage.data(), garbage.size() };
    request_header header{};
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer1, ec), document.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());

    BC_REQUIRE_EQUAL(reader.put(buffer2, ec), 0u);
    BC_REQUIRE(ec == error::jsonrpc_reader_stall);
}

// lax
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(rpc_body_reader__finish__single_value_params__converted_lax_params)
{
    // Tolerated: Electrum non-standard single value params (converted).
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"m","params":"x"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(body.lax_params);
    BC_REQUIRE(!body.lax_batch);
    BC_REQUIRE(body.message.params.has_value());
    BC_REQUIRE(std::holds_alternative<array_t>(body.message.params.value()));
    BC_REQUIRE_EQUAL(std::get<array_t>(body.message.params.value()).size(), one);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__v1_batch_element__delivered_lax_batch)
{
    // Tolerated: btcd non-standard v1 message within a (v2) batch.
    const std::string_view text{ R"([{"id":1,"method":"m","params":[]},)" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer, ec), sub1(text.size()));
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body.changed);
    BC_REQUIRE(body.lax_batch);
    BC_REQUIRE(!body.lax_params);
    BC_REQUIRE(body.message.jsonrpc == version::v1);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__over_length__body_limit)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"test"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(10, ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.put(buffer, ec), text.size());
    BC_REQUIRE(ec == error::http_error_t::body_limit);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__empty_buffer__success_none_consumed)
{
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init({}, ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(reader.put(io::const_buffer{}, ec), zero);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!reader.done());
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__whitespace_after_delivered_close__consumed_done)
{
    const std::string_view close{ "]\n" };
    const std::string_view padding{ " \t\r\n" };
    const io::const_buffer buffer1{ close.data(), close.size() };
    const io::const_buffer buffer2{ padding.data(), padding.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer1, ec), close.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);
    BC_REQUIRE(body.changed);

    BC_REQUIRE_EQUAL(reader.put(buffer2, ec), padding.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(reader.done());
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__garbage_after_delivered_close__batch_malformed)
{
    const std::string_view close{ "]\n" };
    const std::string_view garbage{ " x" };
    const io::const_buffer buffer1{ close.data(), close.size() };
    const io::const_buffer buffer2{ garbage.data(), garbage.size() };
    rpc::request_body::value_type body{};
    body.batch = true;
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init({}, ec);
    BC_REQUIRE(!ec);

    BC_REQUIRE_EQUAL(reader.put(buffer1, ec), close.size());
    BC_REQUIRE(ec == error::http_error_t::need_buffer);

    BC_REQUIRE_EQUAL(reader.put(buffer2, ec), one);
    BC_REQUIRE(ec == error::jsonrpc_batch_malformed);
}

// validation
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(rpc_body_reader__finish__v2_without_method__jsonrpc_requires_method)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec == error::jsonrpc_requires_method);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__numeric_method__not_string)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":42})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec == error::to_json_code(error::json_error_t::json_not_string));
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__v1_boolean_id__jsonrpc_reader_exception)
{
    const std::string_view text{ R"({"id":true,"method":"getblockcount","params":[]})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec == error::jsonrpc_reader_exception);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__v1_without_params__jsonrpc_v1_requires_params)
{
    const std::string_view text{ R"({"id":1,"method":"getblockcount"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec == error::jsonrpc_v1_requires_params);
    BC_REQUIRE(body.message.jsonrpc == version::v1);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__v1_without_id__jsonrpc_v1_requires_id)
{
    const std::string_view text{ R"({"method":"getblockcount","params":[]})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec == error::jsonrpc_v1_requires_id);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__v1_object_params__jsonrpc_v1_requires_array_params)
{
    const std::string_view text{ R"({"jsonrpc":"1.0","id":"curltest","method":"getblockhash","params":{"height":0}})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec == error::jsonrpc_v1_requires_array_params);
    BC_REQUIRE(body.message.params.has_value());
    BC_REQUIRE(std::holds_alternative<object_t>(body.message.params.value()));
    BC_REQUIRE_EQUAL(std::get<object_t>(body.message.params.value()).size(), one);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__v1_array_params__success_expected)
{
    const std::string_view text{ R"({"jsonrpc":"1.0","id":"curltest","method":"getblockhash","params":[1000]})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(body.message.jsonrpc == version::v1);
    BC_REQUIRE_EQUAL(std::get<string_t>(body.message.id.value()), "curltest");
    BC_REQUIRE_EQUAL(body.message.method, "getblockhash");
    BC_REQUIRE_EQUAL(std::get<number_t>(std::get<array_t>(body.message.params.value()).front().value()), 1000.0);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__v2_null_id_object_params__success_expected)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":null,"method":"subtract","params":{"subtrahend":23,"minuend":42}})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(body.message.jsonrpc == version::v2);
    BC_REQUIRE(std::holds_alternative<null_t>(body.message.id.value()));
    const auto& params = std::get<object_t>(body.message.params.value());
    BC_REQUIRE_EQUAL(params.size(), 2u);
    BC_REQUIRE_EQUAL(std::get<number_t>(params.at("subtrahend").value()), 23.0);
    BC_REQUIRE_EQUAL(std::get<number_t>(params.at("minuend").value()), 42.0);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__version_3_0__jsonrpc_invalid_version)
{
    const std::string_view text{ R"({"jsonrpc":"3.0","id":1,"method":"getblockcount","params":[]})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec == error::jsonrpc_invalid_version);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__numeric_version__jsonrpc_invalid_version)
{
    const std::string_view text{ R"({"jsonrpc":2.0,"id":1,"method":"getblockcount","params":[]})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec == error::jsonrpc_invalid_version);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__v2_nested_params__success_expected)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":7,"method":"m","params":[null,true,[1,"a"],{"k":false}]})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(!ec);
    const auto& params = std::get<array_t>(body.message.params.value());
    BC_REQUIRE_EQUAL(params.size(), 4u);
    BC_REQUIRE(std::holds_alternative<null_t>(params.at(0).value()));
    BC_REQUIRE(std::get<boolean_type>(params.at(1).value()));
    const auto& nested = std::get<array_t>(params.at(2).value());
    BC_REQUIRE_EQUAL(nested.size(), 2u);
    BC_REQUIRE_EQUAL(std::get<number_t>(nested.at(0).value()), 1.0);
    BC_REQUIRE_EQUAL(std::get<string_t>(nested.at(1).value()), "a");
    const auto& object = std::get<object_t>(params.at(3).value());
    BC_REQUIRE(!std::get<boolean_type>(object.at("k").value()));
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__non_string_method__error)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":42})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__finish__boolean_id__error)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":true,"method":"m"})" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    request_header header{};
    rpc::request_body::reader reader(header, body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(!ec);

    reader.finish(ec);
    BC_REQUIRE(ec);
}

BC_AUTO_TEST_CASE(rpc_body_reader__put__batch_element_non_string_method__error_undelivered)
{
    const std::string_view text{ R"([{"jsonrpc":"2.0","id":1,"method":42},)" };
    const io::const_buffer buffer{ text.data(), text.size() };
    rpc::request_body::value_type body{};
    rpc::request_body::reader reader(body);
    code ec{};
    reader.init(text.size(), ec);
    BC_REQUIRE(!ec);

    reader.put(buffer, ec);
    BC_REQUIRE(ec);
    BC_REQUIRE(ec != error::http_error_t::need_buffer);
}

BC_AUTO_TEST_SUITE_END()

////#endif // HAVE_SLOW_TESTS
