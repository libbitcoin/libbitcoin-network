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

BOOST_AUTO_TEST_SUITE(json_body_writer_tests)

using namespace network::http;
using namespace network::json;
using value = boost::json::value;
using object = boost::json::object;

bool operator==(const asio::const_buffer& left, const asio::const_buffer& right)
{
    return left.size() == right.size() &&
        is_zero(std::memcmp(left.data(), right.data(), left.size()));
}

bool operator!=(const asio::const_buffer& left, const asio::const_buffer& right)
{
    return !(left == right);
}

BOOST_AUTO_TEST_CASE(json_body_writer__constructor1__default__null_model)
{
    json::body<>::value_type body{};
    json::body<>::writer writer(body);
    BOOST_REQUIRE(boost::get<value>(body.model).is_null());
}

BOOST_AUTO_TEST_CASE(json_body_writer__constructor2__default__null_model)
{
    response_header header{};
    json::body<>::value_type body{};
    json::body<>::writer writer(header, body);
    BOOST_REQUIRE(boost::get<value>(body.model).is_null());
}

BOOST_AUTO_TEST_CASE(json_body_writer__init__default__success)
{
    json::body<>::value_type body{};
    json::body<>::writer writer(body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(json_body_writer__get__null_model__success_expected_no_more)
{
    const std::string_view expected{ "null" };
    const asio::const_buffer out{ expected.data(), expected.size() };
    json::body<>::value_type body{};
    json::body<>::writer writer(body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer.has_value());
    BOOST_REQUIRE(buffer.get().first == out);
    BOOST_REQUIRE(!buffer.get().second);
}

BOOST_AUTO_TEST_CASE(json_body_writer__get__simple_object__success_expected_no_more)
{
    const std::string_view expected{ R"({"key":"value"})" };
    const asio::const_buffer out{ expected.data(), expected.size() };
    json::body<>::value_type body{};
    body.model = object{ { "key", "value" } };
    json::body<>::writer writer(body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer.has_value());
    BOOST_REQUIRE(buffer.get().first == out);
    BOOST_REQUIRE(!buffer.get().second);
}

// A size_hint smaller than the serialized payload forces writer.get() to be
// called multiple times (more == true on all but the last pass). This is the
// chunking that drives socket::async_write's finish flag (finish = !more)
// for websocket body writes: the fix relies on every non-final chunk
// reporting more == true and only the final chunk reporting more == false.
// boost::json::serializer::read() fills the given buffer to capacity on
// every pass but the last, so for this size_hint and payload the split is
// deterministic: four 8-byte-or-fewer chunks.
BOOST_AUTO_TEST_CASE(json_body_writer__get__size_hint_smaller_than_payload__four_chunks_reassemble_with_terminal_no_more)
{
    const std::string_view expected1{ R"({"key":")" };
    const std::string_view expected2{ "xxxxxxxx" };
    const std::string_view expected3{ "xxxxxxxx" };
    const std::string_view expected4{ R"(xxxx"})" };
    const asio::const_buffer out1{ expected1.data(), expected1.size() };
    const asio::const_buffer out2{ expected2.data(), expected2.size() };
    const asio::const_buffer out3{ expected3.data(), expected3.size() };
    const asio::const_buffer out4{ expected4.data(), expected4.size() };

    json::body<>::value_type body{};
    body.model = object{ { "key", std::string(20, 'x') } };
    body.size_hint = 8;
    json::body<>::writer writer(body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!writer.done());

    const auto buffer1 = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer1.has_value());
    BOOST_REQUIRE(buffer1.get().first == out1);
    BOOST_REQUIRE(buffer1.get().second);
    BOOST_REQUIRE(!writer.done());

    const auto buffer2 = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer2.has_value());
    BOOST_REQUIRE(buffer2.get().first == out2);
    BOOST_REQUIRE(buffer2.get().second);
    BOOST_REQUIRE(!writer.done());

    const auto buffer3 = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer3.has_value());
    BOOST_REQUIRE(buffer3.get().first == out3);
    BOOST_REQUIRE(buffer3.get().second);
    BOOST_REQUIRE(!writer.done());

    const auto buffer4 = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer4.has_value());
    BOOST_REQUIRE(buffer4.get().first == out4);
    BOOST_REQUIRE(!buffer4.get().second);
    BOOST_REQUIRE(writer.done());
}

BOOST_AUTO_TEST_SUITE_END()

////#endif // HAVE_SLOW_TESTS
