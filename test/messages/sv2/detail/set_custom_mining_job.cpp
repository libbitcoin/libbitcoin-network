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
#include "../../../test.hpp"

BC_AUTO_TEST_SUITE(sv2_set_custom_mining_job_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_set_custom_mining_job__properties__always__expected)
{
    constexpr auto index = registry::index_of<set_custom_mining_job>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "set_custom_mining_job");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x22u);
    BC_REQUIRE_EQUAL(registry::channels().at(index), true);
}

BC_AUTO_TEST_CASE(sv2_set_custom_mining_job__serialize__sample__expected_frame)
{
    const set_custom_mining_job message{ 0x02020301_u32, 0x03020302_u32, base16_chunk("a3b3c3"), 0x05020304_u32, base16_array("505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f"), 0x07020306_u32, 0x08020307_u32, 0x09020308_u32, base16_chunk("a9b9c9"), 0x0b02030a_u32, base16_chunk("dbebfb0b"), 0x0d02030c_u32, u256s{ base16_array("0d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c"), base16_array("8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabac") } };
    BC_REQUIRE_EQUAL(message.size(), 143u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0080228f0000010302020203020303a3b3c304030205505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f06030207070302080803020903a9b9c90a03020b0400dbebfb0b0c03020d020d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabac");
}

BC_AUTO_TEST_CASE(sv2_set_custom_mining_job__deserialize__sample__round_trip)
{
    const auto message = set_custom_mining_job::deserialize(base16_chunk("010302020203020303a3b3c304030205505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f06030207070302080803020903a9b9c90a03020b0400dbebfb0b0c03020d020d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabac"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "0080228f0000010302020203020303a3b3c304030205505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f06030207070302080803020903a9b9c90a03020b0400dbebfb0b0c03020d020d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabac");
}

BC_AUTO_TEST_CASE(sv2_set_custom_mining_job__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!set_custom_mining_job::deserialize(base16_chunk("010302020203020303a3b3c304030205505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f06030207070302080803020903a9b9c90a03020b0400dbebfb0b0c03020d020d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaab")));
}

BC_AUTO_TEST_CASE(sv2_set_custom_mining_job__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!set_custom_mining_job::deserialize(base16_chunk("010302020203020303a3b3c304030205505152535455565758595a5b5c5d5e5f606162636465666768696a6b6c6d6e6f06030207070302080803020903a9b9c90a03020b0400dbebfb0b0c03020d020d0e0f101112131415161718191a1b1c1d1e1f202122232425262728292a2b2c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5a6a7a8a9aaabac00")));
}

BC_AUTO_TEST_SUITE_END()
