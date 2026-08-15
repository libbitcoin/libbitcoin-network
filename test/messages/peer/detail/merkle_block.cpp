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

BOOST_AUTO_TEST_SUITE(p2p_merkle_block_tests)

using namespace bc::system;
using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(merkle_block__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(merkle_block::command, "merkleblock");
    constexpr auto index = rpc::peer_registry::index_of<merkle_block>();
    BOOST_REQUIRE_EQUAL(rpc::peer_registry::commands().at(index), merkle_block::command);
    BOOST_REQUIRE_EQUAL(merkle_block::version_minimum, level::bip37);
    BOOST_REQUIRE_EQUAL(merkle_block::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(merkle_block__size__default__expected)
{
    constexpr auto expected = zero
        + sizeof(uint32_t)
        + variable_size(zero)
        + variable_size(zero);

    BOOST_REQUIRE_EQUAL(merkle_block{}.size(level::canonical), expected);
}

// With a header present, size() must include the tx count, hashes, and flags,
// not just the header (the terms trail a ternary and require grouping).
BOOST_AUTO_TEST_CASE(merkle_block__size__with_header__includes_all_fields)
{
    const hashes branch{ one_hash, one_hash };
    const data_chunk flags{ 0x1d };
    const merkle_block instance
    {
        to_shared<chain::header>(), 42_u32, branch, flags
    };

    const auto expected = chain::header::serialized_size()
        + sizeof(uint32_t)
        + variable_size(branch.size()) + (branch.size() * hash_size)
        + variable_size(flags.size()) + flags.size();

    BOOST_REQUIRE_EQUAL(instance.size(level::canonical), expected);
}

// A populated merkle_block round-trips through its own wire form.
BOOST_AUTO_TEST_CASE(merkle_block__serialize__with_header__round_trips)
{
    const hashes branch{ one_hash, one_hash };
    const data_chunk flags{ 0x1d };
    const merkle_block instance
    {
        to_shared<chain::header>(), 42_u32, branch, flags
    };

    const auto version = merkle_block::version_maximum;
    data_chunk data(instance.size(version));
    BOOST_REQUIRE(instance.serialize(version, data));

    const auto message = merkle_block::deserialize(version, data);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE(message->header);
    BOOST_REQUIRE_EQUAL(message->transactions, 42_u32);
    BOOST_REQUIRE_EQUAL(message->hashes, branch);
    BOOST_REQUIRE_EQUAL(message->flags, flags);
}

BOOST_AUTO_TEST_SUITE_END()
