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

BC_AUTO_TEST_SUITE(json_body_writer_tests)

using namespace network::http;
using namespace network::json;
using value = system::json::value;
using object = system::json::object;

bool operator==(const io::const_buffer& left, const io::const_buffer& right)
{
    return left.size() == right.size() &&
        is_zero(std::memcmp(left.data(), right.data(), left.size()));
}

bool operator!=(const io::const_buffer& left, const io::const_buffer& right)
{
    return !(left == right);
}

BC_AUTO_TEST_CASE(json_body_writer__constructor1__default__null_model)
{
    http::flat_buffer scratch{ 64 * 1024 };
    json::body<>::value_type body{};
    body.buffer = &scratch;
    json::body<>::writer writer(body);
    BC_REQUIRE(body.model.is_null());
}

BC_AUTO_TEST_CASE(json_body_writer__constructor2__default__null_model)
{
    response_header header{};
    http::flat_buffer scratch{ 64 * 1024 };
    json::body<>::value_type body{};
    body.buffer = &scratch;
    json::body<>::writer writer(header, body);
    BC_REQUIRE(body.model.is_null());
}

BC_AUTO_TEST_CASE(json_body_writer__init__default__success)
{
    http::flat_buffer scratch{ 64 * 1024 };
    json::body<>::value_type body{};
    body.buffer = &scratch;
    json::body<>::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(json_body_writer__get__null_model__success_expected_no_more)
{
    const std::string_view expected{ "null" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    json::body<>::value_type body{};
    body.buffer = &scratch;
    json::body<>::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
}

BC_AUTO_TEST_CASE(json_body_writer__get__simple_object__success_expected_no_more)
{
    const std::string_view expected{ R"({"key":"value"})" };
    const io::const_buffer out{ expected.data(), expected.size() };
    http::flat_buffer scratch{ 64 * 1024 };
    json::body<>::value_type body{};
    body.buffer = &scratch;
    body.model = object{ { "key", "value" } };
    json::body<>::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer.has_value());
    BC_REQUIRE(buffer.value().first == out);
    BC_REQUIRE(!buffer.value().second);
}

// A size_hint smaller than the serialized payload forces writer.get() to be
// called multiple times (more == true on all but the last pass). This is the
// chunking that drives socket::async_write's finish flag (finish = !more)
// for websocket body writes: the fix relies on every non-final chunk
// reporting more == true and only the final chunk reporting more == false.
// serializer::read() fills the given buffer to capacity on
// every pass but the last, so for this size_hint and payload the split is
// deterministic: four 8-byte-or-fewer chunks.
BC_AUTO_TEST_CASE(json_body_writer__get__size_hint_smaller_than_payload__four_chunks_reassemble_with_terminal_no_more)
{
    const std::string_view expected1{ R"({"key":")" };
    const std::string_view expected2{ "xxxxxxxx" };
    const std::string_view expected3{ "xxxxxxxx" };
    const std::string_view expected4{ R"(xxxx"})" };
    const io::const_buffer out1{ expected1.data(), expected1.size() };
    const io::const_buffer out2{ expected2.data(), expected2.size() };
    const io::const_buffer out3{ expected3.data(), expected3.size() };
    const io::const_buffer out4{ expected4.data(), expected4.size() };

    http::flat_buffer scratch{ 64 * 1024 };
    json::body<>::value_type body{};
    body.buffer = &scratch;
    body.model = object{ { "key", std::string(20, 'x') } };
    body.size_hint = 8;
    json::body<>::writer writer(body);
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
    BC_REQUIRE(buffer2.value().second);
    BC_REQUIRE(!writer.done());

    const auto buffer3 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer3.has_value());
    BC_REQUIRE(buffer3.value().first == out3);
    BC_REQUIRE(buffer3.value().second);
    BC_REQUIRE(!writer.done());

    const auto buffer4 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer4.has_value());
    BC_REQUIRE(buffer4.value().first == out4);
    BC_REQUIRE(!buffer4.value().second);
    BC_REQUIRE(writer.done());
}

BC_AUTO_TEST_CASE(json_body_writer__init__null_buffer__bad_alloc)
{
    json::body<>::value_type body{};
    json::body<>::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(ec == error::http_error_t::bad_alloc);
}

BC_AUTO_TEST_CASE(json_body_writer__get__null_buffer_after_init__bad_alloc)
{
    http::flat_buffer scratch{ 64 * 1024 };
    json::body<>::value_type body{};
    body.buffer = &scratch;
    json::body<>::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    body.buffer = nullptr;
    const auto buffer = writer.get(ec);
    BC_REQUIRE(ec == error::http_error_t::bad_alloc);
    BC_REQUIRE(!buffer.has_value());
}

BC_AUTO_TEST_CASE(json_body_writer__get__zero_limit_buffer__buffer_overflow)
{
    http::flat_buffer scratch{ 0 };
    json::body<>::value_type body{};
    body.buffer = &scratch;
    json::body<>::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BC_REQUIRE(ec == error::http_error_t::buffer_overflow);
    BC_REQUIRE(!buffer.has_value());
}

BC_AUTO_TEST_CASE(json_body_writer__get__after_done__end_of_stream)
{
    http::flat_buffer scratch{ 64 * 1024 };
    json::body<>::value_type body{};
    body.buffer = &scratch;
    json::body<>::writer writer(body);
    code ec{};
    writer.init(ec);
    BC_REQUIRE(!ec);

    const auto buffer1 = writer.get(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(buffer1.has_value());
    BC_REQUIRE(writer.done());

    const auto buffer2 = writer.get(ec);
    BC_REQUIRE(ec == error::http_error_t::end_of_stream);
    BC_REQUIRE(!buffer2.has_value());
}

BC_AUTO_TEST_SUITE_END()

////#endif // HAVE_SLOW_TESTS
