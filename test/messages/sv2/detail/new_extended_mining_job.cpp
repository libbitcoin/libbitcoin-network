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

BC_AUTO_TEST_SUITE(sv2_new_extended_mining_job_tests)

using namespace bc::system;
using namespace network::messages::sv2;

BC_AUTO_TEST_CASE(sv2_new_extended_mining_job__properties__always__expected)
{
    constexpr auto index = registry::index_of<new_extended_mining_job>();
    BC_REQUIRE_EQUAL(registry::commands().at(index), "new_extended_mining_job");
    BC_REQUIRE_EQUAL(registry::identifiers().at(index), 0x1fu);
    BC_REQUIRE_EQUAL(registry::channels().at(index), true);
}

BC_AUTO_TEST_CASE(sv2_new_extended_mining_job__serialize__sample__expected_frame)
{
    const new_extended_mining_job message{ 0x02020301_u32, 0x03020302_u32, 0x11223303_u32, 0x05020304_u32, true, u256s{ base16_array("060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425"), base16_array("868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a5") }, base16_chunk("d7e7f707"), base16_chunk("d8e8f808") };
    BC_REQUIRE_EQUAL(message.size(), 95u);
    const auto data = messages::sv2::serialize(message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "00801f5f000001030202020302030103332211040302050102060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a50400d7e7f7070400d8e8f808");
}

BC_AUTO_TEST_CASE(sv2_new_extended_mining_job__deserialize__sample__round_trip)
{
    const auto message = new_extended_mining_job::deserialize(base16_chunk("01030202020302030103332211040302050102060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a50400d7e7f7070400d8e8f808"));
    BC_REQUIRE(message);
    const auto data = messages::sv2::serialize(*message);
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(encode_base16(*data), "00801f5f000001030202020302030103332211040302050102060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a50400d7e7f7070400d8e8f808");
}

BC_AUTO_TEST_CASE(sv2_new_extended_mining_job__deserialize__truncated__nullptr)
{
    BC_REQUIRE(!new_extended_mining_job::deserialize(base16_chunk("01030202020302030103332211040302050102060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a50400d7e7f7070400d8e8f8")));
}

BC_AUTO_TEST_CASE(sv2_new_extended_mining_job__deserialize__trailing_byte__nullptr)
{
    BC_REQUIRE(!new_extended_mining_job::deserialize(base16_chunk("01030202020302030103332211040302050102060708090a0b0c0d0e0f101112131415161718191a1b1c1d1e1f202122232425868788898a8b8c8d8e8f909192939495969798999a9b9c9d9e9fa0a1a2a3a4a50400d7e7f7070400d8e8f80800")));
}

BC_AUTO_TEST_SUITE_END()
