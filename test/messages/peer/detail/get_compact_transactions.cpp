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

BOOST_AUTO_TEST_SUITE(p2p_get_compact_transactions_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(get_compact_transactions__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(get_compact_transactions::command, "getblocktxn");
    constexpr auto index = messages::peer::registry::index_of<get_compact_transactions>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), get_compact_transactions::command);
    BOOST_REQUIRE_EQUAL(get_compact_transactions::version_minimum, level::bip152);
    BOOST_REQUIRE_EQUAL(get_compact_transactions::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(get_compact_transactions__size__default__expected)
{
    constexpr auto expected = system::hash_size +
        variable_size(zero);

    BOOST_REQUIRE_EQUAL(get_compact_transactions{}.size(level::canonical), expected);
}

static const auto block2_hash = system::base16_hash("000000006c02c8ea6e4ff69651f7fcde348fb9d557a06e6957b65552002a7820");
static const auto payload = system::base16_chunk("20782a005255b657696ea057d5b98f34defcf75196f64f6eeac8026c00000000" "03" "00" "fd0301" "01");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(get_compact_transactions__size__three__expected)
{
    const get_compact_transactions message{ block2_hash, { 0, 0x0103, 1 } };
    BOOST_REQUIRE_EQUAL(message.size(level::bip152), payload.size());
}

BOOST_AUTO_TEST_CASE(get_compact_transactions__deserialize1__three__expected)
{
    const auto message = get_compact_transactions::deserialize(level::bip152, payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->block_hash, block2_hash);
    BOOST_REQUIRE_EQUAL(message->indexes.size(), 3u);
    BOOST_REQUIRE_EQUAL(message->indexes[0], 0u);
    BOOST_REQUIRE_EQUAL(message->indexes[1], 0x0103u);
    BOOST_REQUIRE_EQUAL(message->indexes[2], 1u);
}

BOOST_AUTO_TEST_CASE(get_compact_transactions__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!get_compact_transactions::deserialize(level::bip133, payload));
}

BOOST_AUTO_TEST_CASE(get_compact_transactions__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!get_compact_transactions::deserialize(excess_version, payload));
}

BOOST_AUTO_TEST_CASE(get_compact_transactions__deserialize1__truncated__nullptr)
{
    const system::data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BOOST_REQUIRE(!get_compact_transactions::deserialize(level::bip152, data));
}

BOOST_AUTO_TEST_CASE(get_compact_transactions__deserialize2__three__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = get_compact_transactions::deserialize(level::maximum_protocol, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.indexes.size(), 3u);
}

BOOST_AUTO_TEST_CASE(get_compact_transactions__serialize1__three__expected)
{
    const get_compact_transactions message{ block2_hash, { 0, 0x0103, 1 } };
    system::data_chunk data(message.size(level::bip152));
    BOOST_REQUIRE(message.serialize(level::bip152, data));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_SUITE_END()
