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

BOOST_AUTO_TEST_SUITE(ws_frame_tests)

using namespace network::ws;

static const mask rfc_key{ 0x37, 0xfa, 0x21, 0x3d };

// parse_header

BOOST_AUTO_TEST_CASE(frame__parse_header__unmasked_text__expected)
{
    const system::data_chunk data{ 0x81, 0x05, 'H', 'e', 'l', 'l', 'o' };
    frame_header header{};
    code ec{};
    const auto size = parse_header(header, data, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE_EQUAL(size, 2u);
    BOOST_REQUIRE(header.fin);
    BOOST_REQUIRE(header.code == opcode::text);
    BOOST_REQUIRE(!header.masked);
    BOOST_REQUIRE_EQUAL(header.length, 5u);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__masked_text__expected_key)
{
    const system::data_chunk data{ 0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f, 0x4d, 0x51, 0x58 };
    frame_header header{};
    code ec{};
    const auto size = parse_header(header, data, true, ec);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE_EQUAL(size, 6u);
    BOOST_REQUIRE(header.masked);
    BOOST_REQUIRE(header.key == rfc_key);
    BOOST_REQUIRE_EQUAL(header.length, 5u);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__unfragmented_continuation__expected)
{
    const system::data_chunk data{ 0x01, 0x03, 'H', 'e', 'l' };
    frame_header header{};
    code ec{};
    BOOST_REQUIRE_EQUAL(parse_header(header, data, false, ec), 2u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(!header.fin);
    BOOST_REQUIRE(header.code == opcode::text);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__length16__expected)
{
    const system::data_chunk data{ 0x82, 0x7e, 0x01, 0x00 };
    frame_header header{};
    code ec{};
    BOOST_REQUIRE_EQUAL(parse_header(header, data, false, ec), 4u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(header.code == opcode::binary);
    BOOST_REQUIRE_EQUAL(header.length, 256u);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__length64__expected)
{
    const system::data_chunk data{ 0x82, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 };
    frame_header header{};
    code ec{};
    BOOST_REQUIRE_EQUAL(parse_header(header, data, false, ec), 10u);
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE_EQUAL(header.length, 65536u);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__incomplete__need_more)
{
    frame_header header{};
    code ec{};
    BOOST_REQUIRE_EQUAL(parse_header(header, system::data_chunk{ 0x81 }, false, ec), 0u);
    BOOST_REQUIRE_EQUAL(ec, error::need_more);
    BOOST_REQUIRE_EQUAL(parse_header(header, system::data_chunk{ 0x82, 0x7e, 0x01 }, false, ec), 0u);
    BOOST_REQUIRE_EQUAL(ec, error::need_more);
    BOOST_REQUIRE_EQUAL(parse_header(header, system::data_chunk{ 0x81, 0x85, 0x37, 0xfa, 0x21 }, true, ec), 0u);
    BOOST_REQUIRE_EQUAL(ec, error::need_more);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__non_minimal_length__bad_size)
{
    frame_header header{};
    code ec{};
    parse_header(header, system::data_chunk{ 0x82, 0x7e, 0x00, 0x7d }, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_size);
    parse_header(header, system::data_chunk{ 0x82, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff }, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_size);
    parse_header(header, system::data_chunk{ 0x82, 0x7f, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_size);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__reserved_bits__bad_reserved_bits)
{
    frame_header header{};
    code ec{};
    parse_header(header, system::data_chunk{ 0xc1, 0x00 }, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_reserved_bits);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__undefined_opcode__bad_opcode)
{
    frame_header header{};
    code ec{};
    parse_header(header, system::data_chunk{ 0x83, 0x00 }, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_opcode);
    parse_header(header, system::data_chunk{ 0x8b, 0x00 }, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_opcode);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__fragmented_control__bad_control_fragment)
{
    frame_header header{};
    code ec{};
    parse_header(header, system::data_chunk{ 0x09, 0x00 }, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_control_fragment);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__oversized_control__bad_control_size)
{
    frame_header header{};
    code ec{};
    parse_header(header, system::data_chunk{ 0x89, 0x7e, 0x00, 0x7e }, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_control_size);
}

BOOST_AUTO_TEST_CASE(frame__parse_header__mask_by_role__expected)
{
    frame_header header{};
    code ec{};
    parse_header(header, system::data_chunk{ 0x81, 0x00 }, true, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_unmasked_frame);
    parse_header(header, system::data_chunk{ 0x81, 0x80, 0x37, 0xfa, 0x21, 0x3d }, false, ec);
    BOOST_REQUIRE_EQUAL(ec, error::bad_masked_frame);
}

// serialize_header

BOOST_AUTO_TEST_CASE(frame__serialize_header__lengths__expected)
{
    BOOST_REQUIRE_EQUAL(serialize_header(opcode::text, true, 5), (system::data_chunk{ 0x81, 0x05 }));
    BOOST_REQUIRE_EQUAL(serialize_header(opcode::text, false, 125), (system::data_chunk{ 0x01, 0x7d }));
    BOOST_REQUIRE_EQUAL(serialize_header(opcode::binary, true, 126), (system::data_chunk{ 0x82, 0x7e, 0x00, 0x7e }));
    BOOST_REQUIRE_EQUAL(serialize_header(opcode::binary, true, 65535), (system::data_chunk{ 0x82, 0x7e, 0xff, 0xff }));
    BOOST_REQUIRE_EQUAL(serialize_header(opcode::binary, true, 65536), (system::data_chunk{ 0x82, 0x7f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00 }));
    BOOST_REQUIRE_EQUAL(serialize_header(opcode::close, true, 2), (system::data_chunk{ 0x88, 0x02 }));
}

BOOST_AUTO_TEST_CASE(frame__serialize_header__masked__expected)
{
    BOOST_REQUIRE_EQUAL(serialize_header(opcode::pong, true, 5, rfc_key), (system::data_chunk{ 0x8a, 0x85, 0x37, 0xfa, 0x21, 0x3d }));
}

BOOST_AUTO_TEST_CASE(frame__serialize_header__parse_header__round_trip)
{
    const auto data = serialize_header(opcode::binary, false, 70000, rfc_key);
    frame_header header{};
    code ec{};
    BOOST_REQUIRE_EQUAL(parse_header(header, data, true, ec), data.size());
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE(!header.fin);
    BOOST_REQUIRE(header.code == opcode::binary);
    BOOST_REQUIRE_EQUAL(header.length, 70000u);
    BOOST_REQUIRE(header.key == rfc_key);
}

// apply_mask

BOOST_AUTO_TEST_CASE(frame__apply_mask__rfc_vector__expected)
{
    system::data_chunk data{ 0x7f, 0x9f, 0x4d, 0x51, 0x58 };
    apply_mask(data, rfc_key, 0);
    BOOST_REQUIRE_EQUAL(system::to_string(data), "Hello");
}

BOOST_AUTO_TEST_CASE(frame__apply_mask__offset__continues_key)
{
    system::data_chunk head{ 0x7f, 0x9f };
    system::data_chunk tail{ 0x4d, 0x51, 0x58 };
    apply_mask(head, rfc_key, 0);
    apply_mask(tail, rfc_key, 2);
    BOOST_REQUIRE_EQUAL(system::to_string(head) + system::to_string(tail), "Hello");
}

// close

BOOST_AUTO_TEST_CASE(frame__is_valid_close_code__codes__expected)
{
    BOOST_REQUIRE(is_valid_close_code(1000));
    BOOST_REQUIRE(is_valid_close_code(1003));
    BOOST_REQUIRE(is_valid_close_code(1007));
    BOOST_REQUIRE(is_valid_close_code(1013));
    BOOST_REQUIRE(is_valid_close_code(3000));
    BOOST_REQUIRE(is_valid_close_code(4999));
    BOOST_REQUIRE(!is_valid_close_code(0));
    BOOST_REQUIRE(!is_valid_close_code(999));
    BOOST_REQUIRE(!is_valid_close_code(1004));
    BOOST_REQUIRE(!is_valid_close_code(1005));
    BOOST_REQUIRE(!is_valid_close_code(1006));
    BOOST_REQUIRE(!is_valid_close_code(1014));
    BOOST_REQUIRE(!is_valid_close_code(1015));
    BOOST_REQUIRE(!is_valid_close_code(1016));
    BOOST_REQUIRE(!is_valid_close_code(2999));
}

BOOST_AUTO_TEST_CASE(frame__parse_close__empty__no_code)
{
    uint16_t value{ 42 };
    std::string reason{ "x" };
    code ec{};
    BOOST_REQUIRE(parse_close(value, reason, system::data_chunk{}, ec));
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE_EQUAL(value, 0u);
    BOOST_REQUIRE(reason.empty());
}

BOOST_AUTO_TEST_CASE(frame__parse_close__code_and_reason__expected)
{
    uint16_t value{};
    std::string reason{};
    code ec{};
    BOOST_REQUIRE(parse_close(value, reason, system::data_chunk{ 0x03, 0xe8, 'b', 'y', 'e' }, ec));
    BOOST_REQUIRE_EQUAL(ec, error::success);
    BOOST_REQUIRE_EQUAL(value, 1000u);
    BOOST_REQUIRE_EQUAL(reason, "bye");
}

BOOST_AUTO_TEST_CASE(frame__parse_close__invalid__expected_errors)
{
    uint16_t value{};
    std::string reason{};
    code ec{};
    BOOST_REQUIRE(!parse_close(value, reason, system::data_chunk{ 0x03 }, ec));
    BOOST_REQUIRE_EQUAL(ec, error::bad_close_size);
    BOOST_REQUIRE(!parse_close(value, reason, system::data_chunk{ 0x03, 0xed }, ec));
    BOOST_REQUIRE_EQUAL(ec, error::bad_close_code);
    BOOST_REQUIRE(!parse_close(value, reason, system::data_chunk{ 0x03, 0xe8, 0xff }, ec));
    BOOST_REQUIRE_EQUAL(ec, error::bad_close_payload);
}

BOOST_AUTO_TEST_CASE(frame__close_payload__normal__expected)
{
    BOOST_REQUIRE_EQUAL(close_payload(close_code::normal), (system::data_chunk{ 0x03, 0xe8 }));
    BOOST_REQUIRE_EQUAL(close_payload(close_code::internal_error), (system::data_chunk{ 0x03, 0xf3 }));
}

// utf8_validator

BOOST_AUTO_TEST_CASE(frame__utf8_validator__valid__true)
{
    utf8_validator validator{};
    BOOST_REQUIRE(validator.write(system::data_chunk{ 'a', 0xc3, 0xa9, 0xe2, 0x82, 0xac, 0xf0, 0x9f, 0x98, 0x80 }));
    BOOST_REQUIRE(validator.finish());
}

BOOST_AUTO_TEST_CASE(frame__utf8_validator__split_character__true)
{
    utf8_validator validator{};
    BOOST_REQUIRE(validator.write(system::data_chunk{ 0xe2, 0x82 }));
    BOOST_REQUIRE(validator.write(system::data_chunk{ 0xac }));
    BOOST_REQUIRE(validator.finish());
}

BOOST_AUTO_TEST_CASE(frame__utf8_validator__invalid__false)
{
    BOOST_REQUIRE(!utf8_validator{}.write(system::data_chunk{ 0xc0, 0x80 }));
    BOOST_REQUIRE(!utf8_validator{}.write(system::data_chunk{ 0xe0, 0x80, 0x80 }));
    BOOST_REQUIRE(!utf8_validator{}.write(system::data_chunk{ 0xed, 0xa0, 0x80 }));
    BOOST_REQUIRE(!utf8_validator{}.write(system::data_chunk{ 0xf4, 0x90, 0x80, 0x80 }));
    BOOST_REQUIRE(!utf8_validator{}.write(system::data_chunk{ 0xf5 }));
    BOOST_REQUIRE(!utf8_validator{}.write(system::data_chunk{ 0x80 }));
}

BOOST_AUTO_TEST_CASE(frame__utf8_validator__truncated__finish_false)
{
    utf8_validator validator{};
    BOOST_REQUIRE(validator.write(system::data_chunk{ 0xe2, 0x82 }));
    BOOST_REQUIRE(!validator.finish());
}

BOOST_AUTO_TEST_SUITE_END()
