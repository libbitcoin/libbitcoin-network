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

BC_AUTO_TEST_SUITE(rpc_body_writer_tests)

using namespace network::http;
using namespace network::rpc;
using value = system::json::value;

bool operator==(const io::const_buffer& left, const io::const_buffer& right)
{
    return left.size() == right.size() &&
        is_zero(std::memcmp(left.data(), right.data(), left.size()));
}

BC_AUTO_TEST_CASE(rpc_body_writer__construct1__default__default_response_terminated)
{
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    rpc::response_body::writer writer(body);
    BC_REQUIRE(body.message.jsonrpc == version::undefined);
    BC_REQUIRE(!body.message.id.has_value());
    BC_REQUIRE(!body.message.error.has_value());
    BC_REQUIRE(!body.message.result.has_value());
}

BC_AUTO_TEST_CASE(rpc_body_writer__construct2__default__default_response_non_terminated)
{
    response_header header{};
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    rpc::response_body::writer writer(header, body);
    BC_REQUIRE(body.message.jsonrpc == version::undefined);
    BC_REQUIRE(!body.message.id.has_value());
    BC_REQUIRE(!body.message.error.has_value());
    BC_REQUIRE(!body.message.result.has_value());
}

BC_AUTO_TEST_CASE(rpc_body_writer__init__default__success)
{
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    rpc::response_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(rpc_body_writer__init__any_result__jsonrpc_writer_exception)
{
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message.result = value_t{ any_t{ system::to_shared<int>(42) } };
    rpc::response_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(ec == error::jsonrpc_writer_exception);
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__null_response_non_terminated__success_expected_no_more)
{
    const std::string_view expected{ R"({"error":null,"result":null})" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    response_header header{};
    rpc::response_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__simple_response_non_terminated__success_expected_no_more)
{
    const std::string_view expected{ R"({"jsonrpc":"2.0","id":1,"result":true})" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, identity_t{ 1 }, {}, value_t{ true } };
    response_header header{};
    rpc::response_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__v2_notification_result__empty_object)
{
    const std::string_view expected{ R"({"jsonrpc":"2.0"})" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, {}, {}, value_t{ true } };
    response_header header{};
    rpc::response_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__v2_notification_error__empty_object)
{
    const std::string_view expected{ R"({"jsonrpc":"2.0"})" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, {}, result_t{ -32601, "Method not found", {} }, {} };
    response_header header{};
    rpc::response_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__simple_response_terminated__success_expected_with_newline_no_more)
{
    const std::string_view expected{ "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":true}\n" };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, identity_t{ 1 }, {}, value_t{ true } };
    body.terminate = true;
    rpc::response_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());

    using namespace system;
    const std::string data{ pointer_cast<const char>(buffer.value().first.data()), buffer.value().first.size() };
    BC_REQUIRE_EQUAL(data, expected);
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__terminated_fills_bound_less_one__one_chunk_with_newline)
{
    const std::string_view expected{ "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":true}\n" };
    http::flat_buffer scratch{ expected.size() };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, identity_t{ 1 }, {}, value_t{ true } };
    body.terminate = true;
    rpc::response_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());

    using namespace system;
    const std::string data{ pointer_cast<const char>(buffer.value().first.data()), buffer.value().first.size() };
    BC_REQUIRE_EQUAL(data, expected);
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__terminated_fills_bound__two_chunks_within_bound)
{
    const std::string_view expected{ "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":true}\n" };
    const auto bound = sub1(expected.size());
    http::flat_buffer scratch{ bound };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, identity_t{ 1 }, {}, value_t{ true } };
    body.terminate = true;
    rpc::response_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    using namespace system;
    const auto buffer1 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer1.has_value());
    BC_REQUIRE(buffer1.value().second);

    const std::string part1{ pointer_cast<const char>(buffer1.value().first.data()), buffer1.value().first.size() };
    const auto buffer2 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer2.has_value());
    BC_REQUIRE(!buffer2.value().second);
    BC_REQUIRE(writer.done());

    const std::string part2{ pointer_cast<const char>(buffer2.value().first.data()), buffer2.value().first.size() };
    BC_REQUIRE_LE(part1.size(), bound);
    BC_REQUIRE_LE(part2.size(), bound);
    BC_REQUIRE_EQUAL(part1 + part2, expected);
}

// batch
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(rpc_body_writer__get__batch_open_part_terminated__open_prefix_message_no_terminator)
{
    const std::string_view prefix{ "[" };
    const std::string_view expected{ R"({"jsonrpc":"2.0","id":1,"result":true})" };
    const io::const_buffer out1{ prefix.data(), prefix.size() };
    const io::const_buffer out2{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, identity_t{ 1 }, {}, value_t{ true } };
    body.changed = true;
    body.terminate = true;
    rpc::response_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!writer.done());

    const auto buffer1 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer1.has_value());
    BC_REQUIRE(buffer1.value().first == out1);
    BC_REQUIRE(buffer1.value().second);
    BC_REQUIRE(!writer.done());

    const auto buffer2 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer2.has_value());
    BC_REQUIRE(buffer2.value().first == out2);
    BC_REQUIRE(!buffer2.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__batch_continuation_part_terminated__separator_prefix_message_no_terminator)
{
    const std::string_view prefix{ "," };
    const std::string_view expected{ R"({"jsonrpc":"2.0","id":2,"result":true})" };
    const io::const_buffer out1{ prefix.data(), prefix.size() };
    const io::const_buffer out2{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, identity_t{ 2 }, {}, value_t{ true } };
    body.batch = true;
    body.terminate = true;
    rpc::response_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!writer.done());

    const auto buffer1 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer1.has_value());
    BC_REQUIRE(buffer1.value().first == out1);
    BC_REQUIRE(buffer1.value().second);
    BC_REQUIRE(!writer.done());

    const auto buffer2 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer2.has_value());
    BC_REQUIRE(buffer2.value().first == out2);
    BC_REQUIRE(!buffer2.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__batch_close_part_terminated__close_terminator_no_message)
{
    const std::string_view close{ "]" };
    const std::string_view line{ "\n" };
    const io::const_buffer out1{ close.data(), close.size() };
    const io::const_buffer out2{ line.data(), line.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.batch = true;
    body.changed = true;
    body.terminate = true;
    rpc::response_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!writer.done());

    const auto buffer1 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer1.has_value());
    BC_REQUIRE(buffer1.value().first == out1);
    BC_REQUIRE(buffer1.value().second);
    BC_REQUIRE(!writer.done());

    const auto buffer2 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer2.has_value());
    BC_REQUIRE(buffer2.value().first == out2);
    BC_REQUIRE(!buffer2.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__batch_close_part_non_terminated__close_only_no_message)
{
    const std::string_view close{ "]" };
    const io::const_buffer out1{ close.data(), close.size() };
    response_header header{};
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.batch = true;
    body.changed = true;
    rpc::response_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!writer.done());

    const auto buffer1 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer1.has_value());
    BC_REQUIRE(buffer1.value().first == out1);
    BC_REQUIRE(!buffer1.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__batch_open_part_non_terminated__open_prefix_message)
{
    const std::string_view prefix{ "[" };
    const std::string_view expected{ R"({"jsonrpc":"2.0","id":1,"result":true})" };
    const io::const_buffer out1{ prefix.data(), prefix.size() };
    const io::const_buffer out2{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, identity_t{ 1 }, {}, value_t{ true } };
    body.changed = true;
    response_header header{};
    rpc::response_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer1 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer1.has_value());
    BC_REQUIRE(buffer1.value().first == out1);
    BC_REQUIRE(buffer1.value().second);

    const auto buffer2 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer2.has_value());
    BC_REQUIRE(buffer2.value().first == out2);
    BC_REQUIRE(!buffer2.value().second);
    BC_REQUIRE(writer.done());
}

// error
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(rpc_body_writer__get__v2_error_null_id__expected)
{
    const std::string_view expected{ R"({"jsonrpc":"2.0","id":null,"error":{"code":-32700,"message":"Parse error"}})" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, identity_t{ null_t{} }, result_t{ -32700, "Parse error", {} }, {} };
    response_header header{};
    rpc::response_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__get__v2_error_data_string_id__expected)
{
    const std::string_view expected{ R"({"jsonrpc":"2.0","id":"abc","error":{"code":-32601,"message":"Method not found","data":"bogus"}})" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::response_body::value_type body{};
    body.buffer = &scratch;
    body.message = response_t{ version::v2, identity_t{ string_t{ "abc" } }, result_t{ -32601, "Method not found", value_t{ string_t{ "bogus" } } }, {} };
    response_header header{};
    rpc::response_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

// request (notifier)
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(rpc_body_writer__request_get__notification_non_terminated__success_expected_no_more)
{
    const std::string_view expected{ R"({"jsonrpc":"2.0","method":"blockchain.headers.subscribe","params":[{"height":0}]})" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::request_body::value_type body{};
    body.buffer = &scratch;
    body.message = request_t{ version::v2, {}, "blockchain.headers.subscribe", params_t{ array_t{ value_t{ object_t{ { "height", value_t{ uint32_t{ 0 } } } } } } } };
    request_header header{};
    rpc::request_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!writer.done());

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__request_get__notification_terminated__success_expected_with_newline_no_more)
{
    const std::string_view expected_json{ R"({"jsonrpc":"2.0","method":"m","params":["a"]})" };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::request_body::value_type body{};
    body.buffer = &scratch;
    body.message = request_t{ version::v2, {}, "m", params_t{ array_t{ value_t{ string_t{ "a" } } } } };
    body.terminate = true;
    rpc::request_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());

    using namespace system;
    const std::string data{ pointer_cast<const char>(buffer.value().first.data()), buffer.value().first.size() };
    BC_REQUIRE_EQUAL(data, std::string{ expected_json } + "\n");
    BC_REQUIRE(!buffer.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(rpc_body_writer__request_get__request_with_id__expected)
{
    const std::string_view expected{ R"({"jsonrpc":"1.0","id":"curltest","method":"getblockcount","params":[]})" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::request_body::value_type body{};
    body.buffer = &scratch;
    body.message = request_t{ version::v1, identity_t{ string_t{ "curltest" } }, "getblockcount", params_t{ array_t{} } };
    request_header header{};
    rpc::request_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
}

BC_AUTO_TEST_CASE(rpc_body_writer__request_get__empty_request__empty_object)
{
    const std::string_view expected{ "{}" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    rpc::request_body::value_type body{};
    body.buffer = &scratch;
    request_header header{};
    rpc::request_body::writer writer(header, body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
}

BC_AUTO_TEST_CASE(rpc_body_writer__request_init__null_buffer__bad_alloc)
{
    rpc::request_body::value_type body{};
    body.message = request_t{ version::v2, {}, "m", {} };
    rpc::request_body::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(ec == error::http_error_t::bad_alloc);
}

BC_AUTO_TEST_SUITE_END()

////#endif // HAVE_SLOW_TESTS
