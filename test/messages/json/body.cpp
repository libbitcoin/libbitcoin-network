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

BOOST_AUTO_TEST_SUITE(body_tests)

// This is the namespace for body and its types unless otherwise specified.
using namespace network::json;

using namespace boost::system::errc;
const error::boost_code fake_error = make_error_code(bad_message);
const error::boost_code real_error = make_error_code(protocol_error);

struct mock_parser
{
    using buffer_t = string_t;

    bool done{ false };
    std::string written{};
    error::boost_code result{};
    error::boost_code second_write_result{};

    void reset() NOEXCEPT
    {
        done = false;
        result.clear();
        written.clear();
    }

    bool is_done() const NOEXCEPT
    {
        return done;
    }

    bool has_error() const NOEXCEPT
    {
        return !!result;
    }

    error::boost_code get_error() const NOEXCEPT
    {
        return result;
    }

    std::size_t write(std::string_view data) NOEXCEPT
    {
        if (result)
            return {};

        result = second_write_result;
        written += data;
        return data.size();
    }
};

// body<mock_parser>::reader tests.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(body_reader__init__done_parser__resets_parser_and_error)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    parse.done = true;
    parse.result = fake_error;
    parse.written = "42";

    error::boost_code ec{ fake_error };
    reader.init({}, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!parse.done);
    BOOST_REQUIRE(!parse.result);
    BOOST_REQUIRE(parse.written.empty());
}

BOOST_AUTO_TEST_CASE(body_reader__init__not_done_parser__resets_parser_and_error)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    parse.done = false;
    parse.result = fake_error;
    parse.written = "42";

    error::boost_code ec{ fake_error };
    reader.init({}, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!parse.done);
    BOOST_REQUIRE(!parse.result);
    BOOST_REQUIRE(parse.written.empty());
}

BOOST_AUTO_TEST_CASE(body_reader__put__valid_buffer__writes_clears_error_and_returns_size)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    const std::string value{ "{\"jsonrpc\": \"2.0\"}" };
    const asio::const_buffer buffer{ value.data(), value.size() };

    error::boost_code ec{ fake_error };
    const auto size = reader.put(std::array{ buffer }, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, value.size());
    BOOST_REQUIRE_EQUAL(parse.written, value);
}

BOOST_AUTO_TEST_CASE(body_reader__put__multiple_buffers__writes_clears_error_and_returns_size)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    const std::string value1{ "{\"jsonrpc\": \"2.0\", " };
    const std::string value2{ "\"method\": \"getbalance\"}" };
    const std::array buffers
    {
        asio::const_buffer{ value1.data(), value1.size() },
        asio::const_buffer{ value2.data(), value2.size() }
    };

    error::boost_code ec{};
    const auto size = reader.put(buffers, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, value1.size() + value2.size());
    BOOST_REQUIRE_EQUAL(parse.written, value1 + value2);
}

BOOST_AUTO_TEST_CASE(body_reader__put__multiple_buffers_second_write_fails__stops_and_returns_partial_size)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    const std::string value1{ "{\"jsonrpc\": \"2.0\", " };
    const std::string value2{ "\"method\": \"getbalance\"}" };
    const std::array buffers
    {
        asio::const_buffer{ value1.data(), value1.size() },
        asio::const_buffer{ value2.data(), value2.size() }
    };

    parse.second_write_result = fake_error;

    error::boost_code ec{};
    const auto size = reader.put(buffers, ec);
    BOOST_REQUIRE_EQUAL(ec, fake_error);
    BOOST_REQUIRE_EQUAL(size, value1.size());
    BOOST_REQUIRE_EQUAL(parse.written, value1);
}

BOOST_AUTO_TEST_CASE(body_reader__put__parser_done___returns_real_error_and_zero)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    parse.done = true;
    const std::string value{ "{\"method\": \"getbalance\"}" };
    const asio::const_buffer buffer{ value.data(), value.size() };

    error::boost_code ec{};
    const auto size = reader.put(std::array{ buffer }, ec);
    BOOST_REQUIRE_EQUAL(ec, real_error);
    BOOST_REQUIRE_EQUAL(size, zero);
    BOOST_REQUIRE(parse.written.empty());
}

BOOST_AUTO_TEST_CASE(body_reader__put__parser_error___returns_error_and_zero)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    // This buffer isn't acually parsed but we set the fake error.
    const std::string value{ "{\"invalid\": 1}" };
    const asio::const_buffer buffer{ value.data(), value.size() };
    parse.result = fake_error;

    error::boost_code ec{};
    const auto size = reader.put(std::array{ buffer }, ec);
    BOOST_REQUIRE_EQUAL(ec, fake_error);
    BOOST_REQUIRE_EQUAL(size, zero);
    BOOST_REQUIRE(parse.written.empty());
}

BOOST_AUTO_TEST_CASE(body_reader__finish__done_error__returns_error)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    parse.done = true;
    parse.result = fake_error;

    error::boost_code ec{};
    reader.finish(ec);
    BOOST_REQUIRE_EQUAL(ec, fake_error);
}

BOOST_AUTO_TEST_CASE(body_reader__finish__done_no_error__clears_error)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    parse.done = true;

    error::boost_code ec{ fake_error };
    reader.finish(ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(body_reader__finish__not_done_no_error__returns_real_error)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };
    error::boost_code ec{};

    parse.done = false;

    reader.finish(ec);
    BOOST_REQUIRE_EQUAL(ec, real_error);
}

BOOST_AUTO_TEST_CASE(body_reader__finish__not_done_error__returns_error)
{
    mock_parser parse{};
    http::header<true> header{};
    body<mock_parser>::reader reader{ header, parse };

    parse.done = false;
    parse.result = fake_error;

    error::boost_code ec{};
    reader.finish(ec);
    BOOST_REQUIRE_EQUAL(ec, fake_error);
}

// body<mock_parser>::writer tests.
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(body_writer__init__non_empty_buffer__clears_buffer_and_error)
{
    http::header<false> header{};
    body<mock_parser>::writer writer{ header };

    const std::string value{ "{\"result\": 0.5}" };
    const asio::const_buffer buffer{ value.data(), value.size() };

    // Put some data in the buffer.
    error::boost_code ec{};
    writer.put(std::array{ buffer }, ec);

    ec = fake_error;
    writer.init(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(writer.buffer().empty());
}

BOOST_AUTO_TEST_CASE(body_writer__put__valid_buffer__appends_clears_error_and_returns_size)
{
    http::header<false> header{};
    body<mock_parser>::writer writer{ header };

    const std::string value{ "{\"result\": 0.5}" };
    const asio::const_buffer buffer{ value.data(), value.size() };

    error::boost_code ec{ fake_error };
    const auto size = writer.put(std::array{ buffer }, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, value.size());
    BOOST_REQUIRE_EQUAL(writer.buffer(), value);
}

BOOST_AUTO_TEST_CASE(body_writer__put__multiple_buffers__appends_clears_error_and_returns_size)
{
    http::header<false> header{};
    body<mock_parser>::writer writer{ header };

    const std::string value1{ "{\"result\": " };
    const std::string value2{ "0.5}" };
    const std::array buffers
    {
        asio::const_buffer{ value1.data(), value1.size() },
        asio::const_buffer{ value2.data(), value2.size() }
    };

    error::boost_code ec{ fake_error };
    const auto size = writer.put(buffers, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, value1.size() + value2.size());
    BOOST_REQUIRE_EQUAL(writer.buffer(), value1 + value2);
}

BOOST_AUTO_TEST_CASE(body_writer__put__empty_buffer__clears_error_and_returns_zero)
{
    http::header<false> header{};
    body<mock_parser>::writer writer{ header };

    const std::string value{};
    const asio::const_buffer buffer{ value.data(), value.size() };

    error::boost_code ec{ fake_error };
    const auto size = writer.put(std::array{ buffer }, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, zero);
    BOOST_REQUIRE(writer.buffer().empty());
}

BOOST_AUTO_TEST_CASE(body_writer__finish__non_empty_buffer__clears_error)
{
    http::header<false> header{};
    body<mock_parser>::writer writer{ header };

    const std::string value{ "{\"result\": null}" };
    const asio::const_buffer buffer{ value.data(), value.size() };
    error::boost_code ec{ fake_error };

    writer.put(std::array{ buffer }, ec);
    writer.finish(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(writer.buffer(), value);
}

BOOST_AUTO_TEST_CASE(body_writer__finish__empty_buffer__returns_protocol_error)
{
    http::header<false> header{};
    body<mock_parser>::writer writer{ header };

    error::boost_code ec{};
    writer.finish(ec);
    BOOST_REQUIRE_EQUAL(ec, protocol_error);
}

BOOST_AUTO_TEST_CASE(body_writer__buffer__empty_after_init__returns_empty)
{
    http::header<false> header{};
    body<mock_parser>::writer writer{ header };

    error::boost_code ec{ fake_error };
    writer.init(ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(writer.buffer().empty());
}

BOOST_AUTO_TEST_CASE(body_writer__buffer__after_put__returns_appended_data)
{
    http::header<false> header{};
    body<mock_parser>::writer writer{ header };

    const std::string value{ "{\"id\": 1}" };
    const asio::const_buffer buffer{ value.data(), value.size() };

    error::boost_code ec{ fake_error };
    writer.put(std::array{ buffer }, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(writer.buffer(), value);
}

BOOST_AUTO_TEST_SUITE_END()
