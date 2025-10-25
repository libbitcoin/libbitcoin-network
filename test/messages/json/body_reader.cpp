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

BOOST_AUTO_TEST_SUITE(body_reader_tests)

// This is the namespace for body and its types unless otherwise specified.
using namespace network::json;

using namespace boost::system::errc;
const error::boost_code fake_error = make_error_code(bad_message);
const error::boost_code real_error = make_error_code(protocol_error);

struct mock_parser
{
    // Required for body template.
    using value_type = json::request_t;

    // Mocks.
    bool done{ false };
    std::string written{};
    error::boost_code result{};
    error::boost_code second_write_result{};

    // Methods.
    // -----------------------------------------------------------------------

    void reset() NOEXCEPT
    {
        done = false;
        result.clear();
        written.clear();
    }

    size_t write(const std::string_view& data) NOEXCEPT
    {
        if (result)
            return {};

        result = second_write_result;
        written += data;
        return data.size();
    }

    // Properties.
    // -----------------------------------------------------------------------

    operator bool() const NOEXCEPT
    {
        return is_done() && !has_error();
    }

    bool is_done() const NOEXCEPT
    {
        return done;
    }

    bool has_error() const NOEXCEPT
    {
        return !!result;
    }

    error_code get_error() const NOEXCEPT
    {
        return result;
    }

    // unused in tests.
    const std::vector<request_t>& get_parsed() const NOEXCEPT
    {
        static std::vector<request_t> batch{};
        return batch;
    }

    // unused in tests.
    const request_t& get() const NOEXCEPT
    {
        static request_t request{};
        return request;
    }
};

struct mock_serializer
{
    static string_t write(const request_t&) NOEXCEPT
    {
        return {};
    }
    static string_t write(const response_t&) NOEXCEPT
    {
        return {};
    }
};

template <class Parser, class Serializer>
struct mock_body
  : public json::body<Parser, Serializer>
{
    using value_type = mock_parser::value_type;

    struct reader :
      public json::body<Parser, Serializer>::reader
    {
        using base = json::body<Parser, Serializer>::reader;
        using base::base;
        Parser& parser()
        {
            return base::parser_;
        }
    };
};

using mock = mock_body<mock_parser, mock_serializer>;

BOOST_AUTO_TEST_CASE(body_reader__init__done_parser__resets_parser_and_error)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

    reader.parser().done = true;
    reader.parser().result = fake_error;
    reader.parser().written = "42";

    error::boost_code ec{ fake_error };
    reader.init({}, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!reader.parser().done);
    BOOST_REQUIRE(!reader.parser().result);
    BOOST_REQUIRE(reader.parser().written.empty());
}

BOOST_AUTO_TEST_CASE(body_reader__init__not_done_parser__resets_parser_and_error)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

    reader.parser().done = false;
    reader.parser().result = fake_error;
    reader.parser().written = "42";

    error::boost_code ec{ fake_error };
    reader.init({}, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE(!reader.parser().done);
    BOOST_REQUIRE(!reader.parser().result);
    BOOST_REQUIRE(reader.parser().written.empty());
}

BOOST_AUTO_TEST_CASE(body_reader__put__valid_buffer__writes_clears_error_and_returns_size)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

    const std::string value{ "{\"jsonrpc\": \"2.0\"}" };
    const asio::const_buffer buffer{ value.data(), value.size() };

    error::boost_code ec{ fake_error };
    const auto size = reader.put(std::array{ buffer }, ec);
    BOOST_REQUIRE(!ec);
    BOOST_REQUIRE_EQUAL(size, value.size());
    BOOST_REQUIRE_EQUAL(reader.parser().written, value);
}

BOOST_AUTO_TEST_CASE(body_reader__put__multiple_buffers__writes_clears_error_and_returns_size)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

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
    BOOST_REQUIRE_EQUAL(reader.parser().written, value1 + value2);
}

BOOST_AUTO_TEST_CASE(body_reader__put__multiple_buffers_second_write_fails__stops_and_returns_partial_size)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

    const std::string value1{ "{\"jsonrpc\": \"2.0\", " };
    const std::string value2{ "\"method\": \"getbalance\"}" };
    const std::array buffers
    {
        asio::const_buffer{ value1.data(), value1.size() },
        asio::const_buffer{ value2.data(), value2.size() }
    };

    reader.parser().second_write_result = fake_error;

    error::boost_code ec{};
    const auto size = reader.put(buffers, ec);
    BOOST_REQUIRE_EQUAL(ec, fake_error);
    BOOST_REQUIRE_EQUAL(size, value1.size());
    BOOST_REQUIRE_EQUAL(reader.parser().written, value1);
}

BOOST_AUTO_TEST_CASE(body_reader__put__parser_done___returns_real_error_and_zero)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

    reader.parser().done = true;
    const std::string value{ "{\"method\": \"getbalance\"}" };
    const asio::const_buffer buffer{ value.data(), value.size() };

    error::boost_code ec{};
    const auto size = reader.put(std::array{ buffer }, ec);
    BOOST_REQUIRE_EQUAL(ec, real_error);
    BOOST_REQUIRE_EQUAL(size, zero);
    BOOST_REQUIRE(reader.parser().written.empty());
}

BOOST_AUTO_TEST_CASE(body_reader__put__parser_error___returns_error_and_zero)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

    // This buffer isn't acually parsed but we set the fake error.
    const std::string value{ "{\"invalid\": 1}" };
    const asio::const_buffer buffer{ value.data(), value.size() };
    reader.parser().result = fake_error;

    error::boost_code ec{};
    const auto size = reader.put(std::array{ buffer }, ec);
    BOOST_REQUIRE_EQUAL(ec, fake_error);
    BOOST_REQUIRE_EQUAL(size, zero);
    BOOST_REQUIRE(reader.parser().written.empty());
}

BOOST_AUTO_TEST_CASE(body_reader__finish__done_error__returns_error)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

    reader.parser().done = true;
    reader.parser().result = fake_error;

    error::boost_code ec{};
    reader.finish(ec);
    BOOST_REQUIRE_EQUAL(ec, fake_error);
}

BOOST_AUTO_TEST_CASE(body_reader__finish__done_no_error__clears_error)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

    reader.parser().done = true;

    error::boost_code ec{ fake_error };
    reader.finish(ec);
    BOOST_REQUIRE(!ec);
}

BOOST_AUTO_TEST_CASE(body_reader__finish__not_done_no_error__returns_real_error)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };
    error::boost_code ec{};

    reader.parser().done = false;

    reader.finish(ec);
    BOOST_REQUIRE_EQUAL(ec, real_error);
}

BOOST_AUTO_TEST_CASE(body_reader__finish__not_done_error__returns_error)
{
    json::request_t request{};
    http::header<true> header{};
    mock::reader reader{ header, request };

    reader.parser().done = false;
    reader.parser().result = fake_error;

    error::boost_code ec{};
    reader.finish(ec);
    BOOST_REQUIRE_EQUAL(ec, fake_error);
}

BOOST_AUTO_TEST_SUITE_END()
