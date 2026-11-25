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

BC_AUTO_TEST_SUITE(sv2_heading_tests)

using namespace bc::system;
using namespace network::messages::sv2;

static_assert(heading::size() == 6_size);
static_assert(heading::maximum_payload == 0x00ffffff_u32);
static_assert(heading::channel_bit == 0x8000_u16);

// The serialized heading.
static data_chunk write(const heading& head) NOEXCEPT
{
    data_chunk out(heading::size());
    BC_REQUIRE(head.serialize(data_slab{ out }));
    return out;
}

BC_AUTO_TEST_CASE(sv2_heading__serialize__channel_message__little_endian)
{
    BC_REQUIRE_EQUAL(encode_base16(write({ 0x8000, 0x21, 0x00030201 })), "008021010203");
}

BC_AUTO_TEST_CASE(sv2_heading__serialize__short_data__false)
{
    data_chunk out(sub1(heading::size()));
    BC_REQUIRE(!(heading{ 0x8000, 0x21, 0x00030201 }.serialize(data_slab{ out })));
}

BC_AUTO_TEST_CASE(sv2_heading__deserialize__channel_message__expected)
{
    const auto head = heading::deserialize(base16_chunk("008021010203"));
    BC_REQUIRE(head);
    BC_REQUIRE_EQUAL(head->extension_type, 0x8000u);
    BC_REQUIRE_EQUAL(head->message_type, 0x21u);
    BC_REQUIRE_EQUAL(head->message_length, 0x00030201u);
}

BC_AUTO_TEST_CASE(sv2_heading__deserialize__five_bytes__nullptr)
{
    BC_REQUIRE(!heading::deserialize(base16_chunk("0080210102")));
}

BC_AUTO_TEST_CASE(sv2_heading__factory__channel__channel_bit)
{
    const auto head = heading::factory(0x21, true, 36);
    BC_REQUIRE_EQUAL(head.extension_type, 0x8000u);
    BC_REQUIRE_EQUAL(head.message_type, 0x21u);
    BC_REQUIRE_EQUAL(head.message_length, 36u);
}

BC_AUTO_TEST_CASE(sv2_heading__factory__not_channel__zero_extension)
{
    BC_REQUIRE_EQUAL(heading::factory(0x01, false, 6).extension_type, 0u);
}

BC_AUTO_TEST_CASE(sv2_heading__factory__maximum_payload__expected)
{
    BC_REQUIRE_EQUAL(heading::factory(0x01, false, 0x00ffffff).message_length, 0x00ffffffu);
}

BC_AUTO_TEST_CASE(sv2_heading__factory__oversized_payload__default)
{
    const auto head = heading::factory(0x21, true, 0x01000000);
    BC_REQUIRE_EQUAL(head.extension_type, 0u);
    BC_REQUIRE_EQUAL(head.message_type, 0u);
    BC_REQUIRE_EQUAL(head.message_length, 0u);
}

BC_AUTO_TEST_CASE(sv2_heading__channel__bit_set__true)
{
    BC_REQUIRE((heading{ 0x8abc, 0, 0 }.channel()));
}

BC_AUTO_TEST_CASE(sv2_heading__channel__bit_unset__false)
{
    BC_REQUIRE(!(heading{ 0x0abc, 0, 0 }.channel()));
}

BC_AUTO_TEST_CASE(sv2_heading__extension__channel_bit_set__bit_ignored)
{
    BC_REQUIRE_EQUAL((heading{ 0x8abc, 0, 0 }.extension()), 0x0abcu);
}

BC_AUTO_TEST_CASE(sv2_heading__index__registered__expected)
{
    BC_REQUIRE_EQUAL((heading{ 0x8000, 0x21, 0 }.index()), registry::index_of<set_target>());
}

BC_AUTO_TEST_CASE(sv2_heading__index__unregistered_type__unknown)
{
    BC_REQUIRE_EQUAL((heading{ 0x0000, 0x7f, 0 }.index()), registry::unknown);
}

BC_AUTO_TEST_CASE(sv2_heading__index__extension__unknown)
{
    BC_REQUIRE_EQUAL((heading{ 0x8001, 0x21, 0 }.index()), registry::unknown);
}

BC_AUTO_TEST_SUITE_END()
