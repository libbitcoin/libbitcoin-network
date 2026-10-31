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

BOOST_AUTO_TEST_SUITE(ws_reader_tests)

using namespace network::ws;
using event = reader::event;

static std::string to_text(const http::flat_buffer& buffer)
{
    const auto data = buffer.data();
    return { system::pointer_cast<const char>(data.data()), data.size() };
}

// Feed the bytes one at a time, returning the first event or error.
static code feed_bytes(reader& instance, const system::data_chunk& data, http::flat_buffer& message, event& out)
{
    code ec{};
    system::data_chunk pending{};
    for (const auto byte: data)
    {
        pending.push_back(byte);
        const auto used = instance.put(pending, message, out, ec);
        pending.erase(pending.begin(), std::next(pending.begin(), used));
        if (out != event::none || (ec && ec != error::need_more))
            return ec;
    }

    return ec;
}

// messages

BOOST_AUTO_TEST_CASE(reader__put__unmasked_text__message)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    const system::data_chunk data{ 0x81, 0x05, 'H', 'e', 'l', 'l', 'o' };
    BOOST_REQUIRE_EQUAL(instance.put(data, message, out, ec), 7u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::message);
    BOOST_REQUIRE(!instance.binary());
    BOOST_REQUIRE_EQUAL(to_text(message), "Hello");
}

BOOST_AUTO_TEST_CASE(reader__put__masked_text__unmasked_message)
{
    reader instance{ 1024, true };
    http::flat_buffer message{};
    event out{};
    code ec{};
    const system::data_chunk data{ 0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58 };
    BOOST_REQUIRE_EQUAL(instance.put(data, message, out, ec), 11u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::message);
    BOOST_REQUIRE_EQUAL(to_text(message), "Hello");
}

BOOST_AUTO_TEST_CASE(reader__put__masked_text_bytewise__unmasked_message)
{
    reader instance{ 1024, true };
    http::flat_buffer message{};
    event out{};
    const system::data_chunk data{ 0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58 };
    BOOST_REQUIRE_EQUAL(feed_bytes(instance, data, message, out), error::success);
    BOOST_REQUIRE(out == event::message);
    BOOST_REQUIRE_EQUAL(to_text(message), "Hello");
}

BOOST_AUTO_TEST_CASE(reader__put__fragments__joined_message)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    const system::data_chunk data{ 0x01, 0x03, 'H', 'e', 'l', 0x80, 0x02, 'l', 'o' };
    BOOST_REQUIRE_EQUAL(instance.put(data, message, out, ec), 9u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::message);
    BOOST_REQUIRE_EQUAL(to_text(message), "Hello");
}

BOOST_AUTO_TEST_CASE(reader__put__ping_between_fragments__ping_then_message)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    const system::data_chunk data{ 0x01, 0x03, 'H', 'e', 'l', 0x89, 0x02, 'h', 'i', 0x80, 0x02, 'l', 'o' };
    BOOST_REQUIRE_EQUAL(instance.put(data, message, out, ec), 9u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::ping);
    BOOST_REQUIRE_EQUAL(system::to_string(instance.control()), "hi");

    const std::span<const uint8_t> rest{ std::next(data.begin(), 9), data.end() };
    BOOST_REQUIRE_EQUAL(instance.put(rest, message, out, ec), 4u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::message);
    BOOST_REQUIRE_EQUAL(to_text(message), "Hello");
}

BOOST_AUTO_TEST_CASE(reader__put__binary__binary_message)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    const system::data_chunk data{ 0x82, 0x02, 0xff, 0x00 };
    BOOST_REQUIRE_EQUAL(instance.put(data, message, out, ec), 4u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::message);
    BOOST_REQUIRE(instance.binary());
    BOOST_REQUIRE_EQUAL(message.size(), 2u);
}

BOOST_AUTO_TEST_CASE(reader__put__empty_text__empty_message)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    BOOST_REQUIRE_EQUAL(instance.put(system::data_chunk{ 0x81, 0x00 }, message, out, ec), 2u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::message);
    BOOST_REQUIRE(is_zero(message.size()));
}

BOOST_AUTO_TEST_CASE(reader__put__incomplete__need_more)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    BOOST_REQUIRE_EQUAL(instance.put(system::data_chunk{ 0x81, 0x05, 'H', 'e' }, message, out, ec), 4u);
    BOOST_REQUIRE_EQUAL(ec, error::need_more);
    BOOST_REQUIRE(out == event::none);
    BOOST_REQUIRE_EQUAL(to_text(message), "He");
}

// controls

BOOST_AUTO_TEST_CASE(reader__put__masked_pong__pong)
{
    reader instance{ 1024, true };
    http::flat_buffer message{};
    event out{};
    code ec{};
    const system::data_chunk data{ 0x8a, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58 };
    BOOST_REQUIRE_EQUAL(instance.put(data, message, out, ec), 11u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::pong);
    BOOST_REQUIRE_EQUAL(system::to_string(instance.control()), "Hello");
    BOOST_REQUIRE(is_zero(message.size()));
}

BOOST_AUTO_TEST_CASE(reader__put__incomplete_ping__need_more)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    BOOST_REQUIRE_EQUAL(instance.put(system::data_chunk{ 0x89, 0x02, 'h' }, message, out, ec), 2u);
    BOOST_REQUIRE_EQUAL(ec, error::need_more);
    BOOST_REQUIRE(out == event::none);
}

BOOST_AUTO_TEST_CASE(reader__put__close__close_payload)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    BOOST_REQUIRE_EQUAL(instance.put(system::data_chunk{ 0x88, 0x02, 0x03, 0xe8 }, message, out, ec), 4u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::close);
    BOOST_REQUIRE_EQUAL(instance.control(), (system::data_chunk{ 0x03, 0xe8 }));
}

BOOST_AUTO_TEST_CASE(reader__put__empty_close__close)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    BOOST_REQUIRE_EQUAL(instance.put(system::data_chunk{ 0x88, 0x00 }, message, out, ec), 2u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::close);
    BOOST_REQUIRE(instance.control().empty());
}

BOOST_AUTO_TEST_CASE(reader__put__invalid_close__close_error)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x88, 0x01, 0x03 }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_close_size);
    BOOST_REQUIRE(out == event::none);
}

// errors

BOOST_AUTO_TEST_CASE(reader__put__over_maximum__message_too_big)
{
    reader instance{ 4, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x81, 0x05, 'H', 'e', 'l', 'l', 'o' }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::message_too_big);
}

BOOST_AUTO_TEST_CASE(reader__put__fragments_over_maximum__message_too_big)
{
    reader instance{ 4, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x01, 0x03, 'H', 'e', 'l', 0x80, 0x02, 'l', 'o' }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::message_too_big);
}

BOOST_AUTO_TEST_CASE(reader__put__over_buffer__websocket_buffer_overflow)
{
    reader instance{ 1024, false };
    http::flat_buffer message{ 2 };
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x81, 0x05, 'H', 'e', 'l', 'l', 'o' }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::websocket_buffer_overflow);
}

BOOST_AUTO_TEST_CASE(reader__put__leading_continuation__bad_continuation)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x80, 0x01, 'a' }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_continuation);
}

BOOST_AUTO_TEST_CASE(reader__put__data_within_fragments__bad_data_frame)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x01, 0x01, 'a', 0x81, 0x01, 'b' }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_data_frame);
}

BOOST_AUTO_TEST_CASE(reader__put__invalid_utf8_text__bad_frame_payload)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x81, 0x01, 0xff }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_frame_payload);
}

BOOST_AUTO_TEST_CASE(reader__put__truncated_utf8_text__bad_frame_payload)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x81, 0x02, 0xe2, 0x82 }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_frame_payload);
}

BOOST_AUTO_TEST_CASE(reader__put__utf8_split_across_fragments__message)
{
    reader instance{ 1024, false };
    http::flat_buffer message{};
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x01, 0x02, 0xe2, 0x82, 0x80, 0x01, 0xac }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(out == event::message);
    BOOST_REQUIRE_EQUAL(message.size(), 3u);
}

BOOST_AUTO_TEST_CASE(reader__put__unmasked_to_server__bad_unmasked_frame)
{
    reader instance{ 1024, true };
    http::flat_buffer message{};
    event out{};
    code ec{};
    instance.put(system::data_chunk{ 0x81, 0x01, 'a' }, message, out, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_unmasked_frame);
}

BOOST_AUTO_TEST_SUITE_END()
