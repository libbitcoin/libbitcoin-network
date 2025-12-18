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
#include "../test.hpp"

#if defined(HAVE_SLOW_TESTS)

BOOST_AUTO_TEST_SUITE(rpc_body_writer_tests)

using namespace network::http;
using namespace network::rpc;
using value = boost::json::value;

bool operator==(const asio::const_buffer& left, const asio::const_buffer& right)
{
    return left.size() == right.size() &&
        is_zero(std::memcmp(left.data(), right.data(), left.size()));
}

BOOST_AUTO_TEST_CASE(rpc_body_writer__construct1__default__default_response_terminated)
{
    rpc::body::value_type body{};
    rpc::body::writer writer(body);
    BOOST_REQUIRE(body.response.jsonrpc == version::undefined);
    BOOST_REQUIRE(!body.response.id.has_value());
    BOOST_REQUIRE(!body.response.error.has_value());
    BOOST_REQUIRE(!body.response.result.has_value());
}

BOOST_AUTO_TEST_CASE(rpc_body_writer__construct2__default__default_response_non_terminated)
{
    response_header header{};
    rpc::body::value_type body{};
    rpc::body::writer writer(header, body);
    BOOST_REQUIRE(body.response.jsonrpc == version::undefined);
    BOOST_REQUIRE(!body.response.id.has_value());
    BOOST_REQUIRE(!body.response.error.has_value());
    BOOST_REQUIRE(!body.response.result.has_value());
}

BOOST_AUTO_TEST_CASE(rpc_body_writer__init__default__success)
{
    rpc::body::value_type body{};
    rpc::body::writer writer(body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(rpc_body_writer__get__null_response_non_terminated__success_expected_no_more)
{
    const std::string_view expected{ R"({"error":null})" };
    const asio::const_buffer out{ expected.data(), expected.size() };
    rpc::body::value_type body{};
    response_header header{};
    rpc::body::writer writer(header, body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer.has_value());
    BOOST_REQUIRE(buffer.get().first == out);
    BOOST_REQUIRE(!buffer.get().second);
}

BOOST_AUTO_TEST_CASE(rpc_body_writer__get__simple_response_non_terminated__success_expected_no_more)
{
    const std::string_view expected{ R"({"jsonrpc":"2.0","id":1,"result":true})" };
    const asio::const_buffer out{ expected.data(), expected.size() };
    rpc::body::value_type body{};
    body.response = response_t{ version::v2, identity_t{ 1 }, {}, value_t{ true } };
    response_header header{};
    rpc::body::writer writer(header, body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);

    const auto buffer = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer.has_value());
    BOOST_REQUIRE(buffer.get().first == out);
    BOOST_REQUIRE(!buffer.get().second);
}

BOOST_AUTO_TEST_CASE(rpc_body_writer__get__simple_response_terminated__success_expected_with_newline_no_more)
{
    const std::string_view expected_json{ R"({"jsonrpc":"2.0","id":1,"result":true})" };
    const std::string_view expected_newline{ "\n" };
    rpc::body::value_type body{};
    body.response = response_t{ version::v2, identity_t{ 1 }, {}, value_t{ true } };
    rpc::body::writer writer(body);
    boost_code ec{};
    writer.init(ec);
    BOOST_REQUIRE(!ec);

    const auto buffer1 = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer1.has_value());

    const asio::const_buffer value1{ expected_json.data(), expected_json.size() };
    BOOST_REQUIRE(buffer1.get().first == value1);
    BOOST_REQUIRE(buffer1.get().second);

    const auto buffer2 = writer.get(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(buffer2.has_value());

    using namespace system;
    const std::string data{ pointer_cast<const char>(buffer2.get().first.data()), buffer2.get().first.size() };
    BOOST_REQUIRE_EQUAL(data, expected_newline);
    BOOST_REQUIRE(!buffer2.get().second);
}

BOOST_AUTO_TEST_SUITE_END()

#endif // HAVE_SLOW_TESTS
