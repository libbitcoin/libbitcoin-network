/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(headers_tests)

using namespace bc::system;
using namespace bc::network::messages;

BOOST_AUTO_TEST_CASE(headers__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(headers::command, "headers");
    BOOST_REQUIRE(headers::id == identifier::headers);
    BOOST_REQUIRE_EQUAL(headers::version_minimum, level::headers_protocol);
    BOOST_REQUIRE_EQUAL(headers::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(headers__size__default__expected)
{
    constexpr auto expected = variable_size(zero);
    BOOST_REQUIRE_EQUAL(headers{}.size(level::headers_protocol), expected);
}

BOOST_AUTO_TEST_CASE(headers__size__two__expected)
{
    // Each header must trail a zero byte (yes, it's stoopid).
    constexpr auto values = two * (chain::header::serialized_size() + sizeof(uint8_t));
    constexpr auto expected = variable_size(two) + values;
    const headers message{ { to_shared<chain::header>(), to_shared<chain::header>() } };
    BOOST_REQUIRE_EQUAL(message.size(level::headers_protocol), expected);
}

// serialize2

BOOST_AUTO_TEST_CASE(headers__serialize2__default__empty)
{
    const headers message{};
    data_chunk data{};
    system::write::bytes::data sink(data);
    message.serialize(level::headers_protocol, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE(data.empty());
}

BOOST_AUTO_TEST_CASE(headers__serialize2__insufficient_version__sink_false)
{
    const headers message{};
    data_chunk data{};
    system::write::bytes::data sink(data);
    message.serialize(level::canonical, sink);
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE(data.empty());
}

BOOST_AUTO_TEST_CASE(headers__serialize2__overflow__sink_false)
{
    const headers message
    {
        chain::header_cptrs
        {
            to_shared<chain::header>()
        }
    };

    data_chunk data{};
    system::write::bytes::copy sink(data);
    message.serialize(level::headers_protocol, sink);
    BOOST_REQUIRE(!sink);
}

BOOST_AUTO_TEST_CASE(headers__serialize2__default_header__expected)
{
    const headers message
    {
        chain::header_cptrs
        {
            to_shared<chain::header>()
        }
    };

    data_chunk data{};
    system::write::bytes::data sink(data);
    message.serialize(level::headers_protocol, sink);
    sink.flush();
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, base16_chunk(
        "01"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000"));
}

// serialize1

BOOST_AUTO_TEST_CASE(headers__serialize1__overflow__sink_false)
{
    const headers message
    {
        chain::header_cptrs
        {
            to_shared<chain::header>()
        }
    };

    data_chunk data{};
    BOOST_REQUIRE(!message.serialize(level::headers_protocol, data));
}

BOOST_AUTO_TEST_CASE(headers__serialize1__headers__expected)
{
    const headers message
    {
        chain::header_cptrs
        {
            to_shared<chain::header>(),
            to_shared<chain::header>(
            {
                10,
                { 42 },
                { 24 },
                531234,
                6523454,
                68644
            })
        }
    };

    data_chunk data{};
    data.resize(message.size(level::headers_protocol));
    BOOST_REQUIRE(message.serialize(level::headers_protocol, data));
    BOOST_REQUIRE_EQUAL(data, base16_chunk(
        "02"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "0a0000002a00000000000000000000000000000000000000000000000000000000000000180000000"
        "0000000000000000000000000000000000000000000000000000000221b08003e8a6300240c010000"));
}

// deserialize2

BOOST_AUTO_TEST_CASE(headers__deserialize2__empty__empty)
{
    const headers expected{};
    const auto data = base16_chunk("00");
    system::read::bytes::copy source(data);
    const auto message = headers::deserialize(level::headers_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(message.header_ptrs.empty());
}

BOOST_AUTO_TEST_CASE(headers__deserialize2__insufficient_version__source_false)
{
    const auto data = base16_chunk("00");
    system::read::bytes::copy source(data);
    const auto message = headers::deserialize(level::canonical, source);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_CASE(headers__deserialize2__underflow__source_false)
{
    const auto data = base16_chunk("01");
    system::read::bytes::copy source(data);
    const auto message = headers::deserialize(level::headers_protocol, source);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_CASE(headers__deserialize2__default_header__expected)
{
    const headers expected
    {
        chain::header_cptrs
        {
            to_shared<chain::header>()
        }
    };

    const auto data = base16_chunk("01"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    system::read::bytes::copy source(data);
    const auto message = headers::deserialize(level::headers_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.header_ptrs.size(), one);
    BOOST_REQUIRE(*message.header_ptrs.front() == *expected.header_ptrs.front());
}

BOOST_AUTO_TEST_CASE(headers__deserialize1__underflow__nullptr)
{
    const auto data = base16_chunk("01");
    const auto message = headers::deserialize(level::headers_protocol, data);
    BOOST_REQUIRE(!message);
}

// deserialize1

BOOST_AUTO_TEST_CASE(headers__deserialize1__headers__expected)
{
    const headers expected
    {
        chain::header_cptrs
        {
            to_shared<chain::header>(),
            to_shared<chain::header>(
            {
                10,
                { 42 },
                { 24 },
                531234,
                6523454,
                68644
            })
        }
    };

    const auto data = base16_chunk("02"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "0a0000002a00000000000000000000000000000000000000000000000000000000000000180000000"
        "0000000000000000000000000000000000000000000000000000000221b08003e8a6300240c010000");
    const auto message = headers::deserialize(level::headers_protocol, data);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->header_ptrs.size(), two);
    BOOST_REQUIRE(*message->header_ptrs.front() == *expected.header_ptrs.front());
    BOOST_REQUIRE(*message->header_ptrs.back() == *expected.header_ptrs.back());
}

BOOST_AUTO_TEST_SUITE_END()
