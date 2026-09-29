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

BOOST_AUTO_TEST_SUITE(p2p_compact_transactions_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(ccompact_transactions__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(compact_transactions::command, "blocktxn");
    constexpr auto index = messages::peer::registry::index_of<compact_transactions>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), compact_transactions::command);
    BOOST_REQUIRE_EQUAL(compact_transactions::version_minimum, level::bip152);
    BOOST_REQUIRE_EQUAL(compact_transactions::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(compact_transactions__size__default__expected)
{
    constexpr auto expected = system::hash_size
        + variable_size(zero);

    BOOST_REQUIRE_EQUAL(compact_transactions{}.size(level::canonical, true), expected);
    BOOST_REQUIRE_EQUAL(compact_transactions{}.size(level::canonical, false), expected);
}

// bitcoin/src/test/data/blockfilters.json (testnet block 2)
static const auto block2_hash = system::base16_hash("000000006c02c8ea6e4ff69651f7fcde348fb9d557a06e6957b65552002a7820");
static const auto coinbase2 = system::base16_chunk("01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff0e0432e7494d010e062f503253482fffffffff0100f2052a010000002321038a7f6ef1c8ca0c588aa53fa860128077c9e6c11e6830f4d7ee4e763a56b7718fac00000000");
static const auto coinbase2_hash = system::base16_hash("20222eb90f5895556926c112bb5aa0df4ab5abc3107e21a6950aec3b2e3541e2");
static const auto payload = system::build_chunk({ system::base16_chunk("20782a005255b657696ea057d5b98f34defcf75196f64f6eeac8026c00000000" "01"), coinbase2 });
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BOOST_AUTO_TEST_CASE(compact_transactions__size__coinbase__expected)
{
    const compact_transactions message{ block2_hash, { system::to_shared<system::chain::transaction>(coinbase2, true) } };
    BOOST_REQUIRE_EQUAL(message.size(level::bip152, true), payload.size());
    BOOST_REQUIRE_EQUAL(message.size(level::bip152, false), payload.size());
}

BOOST_AUTO_TEST_CASE(compact_transactions__deserialize1__coinbase__expected)
{
    const auto message = compact_transactions::deserialize(level::bip152, payload, true);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->block_hash, block2_hash);
    BOOST_REQUIRE_EQUAL(message->transaction_ptrs.size(), one);
    BOOST_REQUIRE_EQUAL(message->transaction_ptrs.front()->hash(false), coinbase2_hash);
}

BOOST_AUTO_TEST_CASE(compact_transactions__deserialize1__insufficient_version__nullptr)
{
    BOOST_REQUIRE(!compact_transactions::deserialize(level::bip133, payload, true));
}

BOOST_AUTO_TEST_CASE(compact_transactions__deserialize1__excess_version__nullptr)
{
    BOOST_REQUIRE(!compact_transactions::deserialize(excess_version, payload, true));
}

BOOST_AUTO_TEST_CASE(compact_transactions__deserialize1__truncated__nullptr)
{
    const system::data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BOOST_REQUIRE(!compact_transactions::deserialize(level::bip152, data, false));
}

BOOST_AUTO_TEST_CASE(compact_transactions__deserialize2__coinbase__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = compact_transactions::deserialize(level::maximum_protocol, source, false);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE_EQUAL(message.transaction_ptrs.size(), one);
}

BOOST_AUTO_TEST_CASE(compact_transactions__serialize1__coinbase__expected)
{
    const auto message = compact_transactions::deserialize(level::bip152, payload, true);
    BOOST_REQUIRE(message);

    system::data_chunk data(message->size(level::bip152, true));
    BOOST_REQUIRE(message->serialize(level::bip152, data, true));
    BOOST_REQUIRE_EQUAL(data, payload);
}

BOOST_AUTO_TEST_SUITE_END()
