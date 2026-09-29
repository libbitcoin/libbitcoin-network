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

BOOST_AUTO_TEST_SUITE(tls_codec_tests)

using namespace bc::system;
using reader = network::tls::reader;
using writer = network::tls::writer;

static data_chunk chunk(const const_byte_span& bytes)
{
    return { bytes.begin(), bytes.end() };
}

BOOST_AUTO_TEST_CASE(tls_codec__reader__integers__big_endian)
{
    const auto data = base16_chunk("01" "0203" "040506");
    reader source{ data };
    BOOST_REQUIRE_EQUAL(source.read_8(), 0x01u);
    BOOST_REQUIRE_EQUAL(source.read_16(), 0x0203u);
    BOOST_REQUIRE_EQUAL(source.read_24(), 0x040506u);
    BOOST_REQUIRE(source.is_complete());
}

BOOST_AUTO_TEST_CASE(tls_codec__reader__vectors__contents)
{
    const auto data = base16_chunk("02aabb" "0001cc" "000002ddee");
    reader source{ data };
    BOOST_REQUIRE_EQUAL(chunk(source.read_vector_8()), base16_chunk("aabb"));
    BOOST_REQUIRE_EQUAL(chunk(source.read_vector_16()), base16_chunk("cc"));
    BOOST_REQUIRE_EQUAL(chunk(source.read_vector_24()), base16_chunk("ddee"));
    BOOST_REQUIRE(source.is_complete());
}

BOOST_AUTO_TEST_CASE(tls_codec__reader__short_vector__invalid)
{
    const auto data = base16_chunk("03aabb");
    reader source{ data };
    BOOST_REQUIRE(source.read_vector_8().empty());
    BOOST_REQUIRE(!source);
    BOOST_REQUIRE(!source.is_complete());
    BOOST_REQUIRE_EQUAL(source.read_16(), 0u);
}

BOOST_AUTO_TEST_CASE(tls_codec__reader__short_integers__invalid)
{
    const auto data = base16_chunk("01");
    reader source16{ data };
    BOOST_REQUIRE_EQUAL(source16.read_16(), 0u);
    BOOST_REQUIRE(!source16);

    reader source24{ data };
    BOOST_REQUIRE_EQUAL(source24.read_24(), 0u);
    BOOST_REQUIRE(!source24);

    reader source8{ {} };
    BOOST_REQUIRE_EQUAL(source8.read_8(), 0u);
    BOOST_REQUIRE(!source8);
}

BOOST_AUTO_TEST_CASE(tls_codec__reader__invalid_length_prefix__empty)
{
    const auto data = base16_chunk("00");
    reader source16{ data };
    BOOST_REQUIRE(source16.read_vector_16().empty());
    BOOST_REQUIRE(!source16);

    reader source24{ data };
    BOOST_REQUIRE(source24.read_vector_24().empty());
    BOOST_REQUIRE(!source24);
}

BOOST_AUTO_TEST_CASE(tls_codec__writer__integers_and_vectors__expected)
{
    writer sink{};
    sink.write_8(0x01);
    sink.write_16(0x0203);
    sink.write_24(0x040506);
    sink.write_bytes(base16_chunk("07"));
    sink.write_vector_8(base16_chunk("aabb"));
    sink.write_vector_16(base16_chunk("cc"));
    sink.write_vector_24(base16_chunk("ddee"));
    BOOST_REQUIRE_EQUAL(sink.data(), base16_chunk("01" "0203" "040506" "07" "02aabb" "0001cc" "000002ddee"));
}

BOOST_AUTO_TEST_SUITE_END()
