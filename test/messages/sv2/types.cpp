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

BC_AUTO_TEST_SUITE(sv2_types_tests)

using namespace system;
using namespace network::messages::sv2;

// True if the reader consumes the bytes whole and validly.
template <typename Reader>
static bool reads(const data_chunk& data, Reader&& reader) NOEXCEPT
{
    system::istream stream{ data };
    byte_reader source{ stream };
    reader(source);
    return source && source.is_exhausted();
}

// The value the reader obtains from the bytes.
template <typename Reader>
static auto read(const data_chunk& data, Reader&& reader) NOEXCEPT
{
    system::istream stream{ data };
    byte_reader source{ stream };
    return reader(source);
}

// BOOL

BC_AUTO_TEST_CASE(sv2_types__read_bool__one__true)
{
    BC_REQUIRE(read(base16_chunk("01"), read_bool));
}

BC_AUTO_TEST_CASE(sv2_types__read_bool__zero__false)
{
    BC_REQUIRE(!read(base16_chunk("00"), read_bool));
}

BC_AUTO_TEST_CASE(sv2_types__read_bool__high_bits_set__least_significant_bit)
{
    BC_REQUIRE(!read(base16_chunk("fe"), read_bool));
    BC_REQUIRE(read(base16_chunk("ff"), read_bool));
}

// F32

BC_AUTO_TEST_CASE(sv2_types__read_f32__one_and_half__expected)
{
    BC_REQUIRE_EQUAL(read(base16_chunk("0000c03f"), read_f32), 1.5f);
}

BC_AUTO_TEST_CASE(sv2_types__read_f32__positive_zero__valid)
{
    BC_REQUIRE(reads(base16_chunk("00000000"), read_f32));
}

BC_AUTO_TEST_CASE(sv2_types__read_f32__negative_zero__invalid)
{
    BC_REQUIRE(!reads(base16_chunk("00000080"), read_f32));
}

BC_AUTO_TEST_CASE(sv2_types__read_f32__negative__invalid)
{
    BC_REQUIRE(!reads(base16_chunk("0000c0bf"), read_f32));
}

BC_AUTO_TEST_CASE(sv2_types__read_f32__infinity__invalid)
{
    BC_REQUIRE(!reads(base16_chunk("0000807f"), read_f32));
}

BC_AUTO_TEST_CASE(sv2_types__read_f32__nan__invalid)
{
    BC_REQUIRE(!reads(base16_chunk("0000c07f"), read_f32));
}

// STR0_255

BC_AUTO_TEST_CASE(sv2_types__read_str0_255__printable__expected)
{
    BC_REQUIRE_EQUAL(read(base16_chunk("0320417e"), read_str0_255), " A~");
}

BC_AUTO_TEST_CASE(sv2_types__read_str0_255__empty__valid)
{
    BC_REQUIRE(reads(base16_chunk("00"), read_str0_255));
}

BC_AUTO_TEST_CASE(sv2_types__read_str0_255__control_character__invalid)
{
    BC_REQUIRE(!reads(base16_chunk("02411f"), read_str0_255));
}

BC_AUTO_TEST_CASE(sv2_types__read_str0_255__delete_character__invalid)
{
    BC_REQUIRE(!reads(base16_chunk("02417f"), read_str0_255));
}

BC_AUTO_TEST_CASE(sv2_types__read_str0_255__short__invalid)
{
    BC_REQUIRE(!reads(base16_chunk("0341"), read_str0_255));
}

// B0_32

BC_AUTO_TEST_CASE(sv2_types__read_b0_32__thirty_two__valid)
{
    BC_REQUIRE(reads(splice(base16_chunk("20"), data_chunk(32, 0x42)), read_b0_32));
}

BC_AUTO_TEST_CASE(sv2_types__read_b0_32__thirty_three__invalid)
{
    BC_REQUIRE(!reads(splice(base16_chunk("21"), data_chunk(33, 0x42)), read_b0_32));
}

// B0_64K

BC_AUTO_TEST_CASE(sv2_types__read_b0_64k__two_bytes__little_endian_count)
{
    BC_REQUIRE_EQUAL(read(base16_chunk("0200abcd"), read_b0_64k), base16_chunk("abcd"));
}

// OPTION[U32]

BC_AUTO_TEST_CASE(sv2_types__read_option_u32__empty__no_value)
{
    BC_REQUIRE(!read(base16_chunk("00"), read_option_u32).has_value());
}

BC_AUTO_TEST_CASE(sv2_types__read_option_u32__one__value)
{
    BC_REQUIRE_EQUAL(read(base16_chunk("0104030201"), read_option_u32).value(), 0x01020304u);
}

BC_AUTO_TEST_CASE(sv2_types__read_option_u32__two__invalid)
{
    BC_REQUIRE(!reads(base16_chunk("020403020104030201"), read_option_u32));
}

// SEQ0_255[U256], SEQ0_64K[U32]

BC_AUTO_TEST_CASE(sv2_types__read_seq0_255_u256__empty__empty)
{
    BC_REQUIRE(read(base16_chunk("00"), read_seq0_255_u256).empty());
}

BC_AUTO_TEST_CASE(sv2_types__read_seq0_255_u256__count_exceeds_data__invalid)
{
    BC_REQUIRE(!reads(splice(base16_chunk("02"), data_chunk(32, 0x42)), read_seq0_255_u256));
}

BC_AUTO_TEST_CASE(sv2_types__read_seq0_64k_u32__two__expected)
{
    BC_REQUIRE((read(base16_chunk("02000700000009000000"), read_seq0_64k_u32) == std::vector<uint32_t>{ 7, 9 }));
}

BC_AUTO_TEST_SUITE_END()
