/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(heading_tests)

using namespace bc::network::messages;

// deserialize
// serialize

static_assert(heading::maximum_payload(0, true) == 4'000'000_size);
static_assert(heading::maximum_payload(0, false) == 1'800'003_size);
static_assert(heading::maximum_payload(max_uint32, true) == 4'000'000_size);
static_assert(heading::maximum_payload(max_uint32, false) == 1'800'003_size);

constexpr auto empty_hash = system::sha256::double_hash(system::sha256::ablocks_t<zero>{});
constexpr auto empty_checksum = system::from_little_endian<uint32_t>(empty_hash);

BOOST_AUTO_TEST_CASE(heading__size__always__expected)
{
    constexpr auto expected = sizeof(uint32_t)
        + heading::command_size
        + sizeof(uint32_t)
        + sizeof(uint32_t);

    BOOST_REQUIRE_EQUAL(heading::size(), expected);
}

BOOST_AUTO_TEST_CASE(heading__address_id__always__expected)
{
    const auto instance = heading{ 0u, address::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == address::id);
}

BOOST_AUTO_TEST_CASE(heading__alert_id__always__expected)
{
    const auto instance = heading{ 0u, alert::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == alert::id);
}

BOOST_AUTO_TEST_CASE(heading__block_id__always__expected)
{
    const auto instance = heading{ 0u, block::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == block::id);
}

BOOST_AUTO_TEST_CASE(heading__bloom_filter_add_id__always__expected)
{
    const auto instance = heading{ 0u, bloom_filter_add::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == bloom_filter_add::id);
}

BOOST_AUTO_TEST_CASE(heading__bloom_filter_clear_id__always__expected)
{
    const auto instance = heading{ 0u, bloom_filter_clear::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == bloom_filter_clear::id);
}

BOOST_AUTO_TEST_CASE(heading__bloom_filter_load_id__always__expected)
{
    const auto instance = heading{ 0u, bloom_filter_load::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == bloom_filter_load::id);
}

BOOST_AUTO_TEST_CASE(heading__client_filter_id__always__expected)
{
    const auto instance = heading{ 0u, client_filter::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == client_filter::id);
}

BOOST_AUTO_TEST_CASE(heading__client_filter_checkpoint_id__always__expected)
{
    const auto instance = heading{ 0u, client_filter_checkpoint::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == client_filter_checkpoint::id);
}

BOOST_AUTO_TEST_CASE(heading__client_filter_headers_id__always__expected)
{
    const auto instance = heading{ 0u, client_filter_headers::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == client_filter_headers::id);
}

BOOST_AUTO_TEST_CASE(heading__compact_block_id__always__expected)
{
    const auto instance = heading{ 0u, compact_block::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == compact_block::id);
}

BOOST_AUTO_TEST_CASE(heading__compact_transactions_id__always__expected)
{
    const auto instance = heading{ 0u, compact_transactions::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == compact_transactions::id);
}

BOOST_AUTO_TEST_CASE(heading__fee_filter_id__always__expected)
{
    const auto instance = heading{ 0u, fee_filter::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == fee_filter::id);
}

BOOST_AUTO_TEST_CASE(heading__get_address_id__always__expected)
{
    const auto instance = heading{ 0u, get_address::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == get_address::id);
}

BOOST_AUTO_TEST_CASE(heading__get_blocks_id__always__expected)
{
    const auto instance = heading{ 0u, get_blocks::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == get_blocks::id);
}

BOOST_AUTO_TEST_CASE(heading__get_client_filter_checkpoint_id__always__expected)
{
    const auto instance = heading{ 0u, get_client_filter_checkpoint::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == get_client_filter_checkpoint::id);
}

BOOST_AUTO_TEST_CASE(heading__aget_client_filter_headers_id__always__expected)
{
    const auto instance = heading{ 0u, get_client_filter_headers::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == get_client_filter_headers::id);
}

BOOST_AUTO_TEST_CASE(heading__get_client_filters_id__always__expected)
{
    const auto instance = heading{ 0u, get_client_filters::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == get_client_filters::id);
}

BOOST_AUTO_TEST_CASE(heading__get_compact_transactions_id__always__expected)
{
    const auto instance = heading{ 0u, get_compact_transactions::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == get_compact_transactions::id);
}

BOOST_AUTO_TEST_CASE(heading__get_data_id__always__expected)
{
    const auto instance = heading{ 0u, get_data::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == get_data::id);
}

BOOST_AUTO_TEST_CASE(heading__get_headers_id__always__expected)
{
    const auto instance = heading{ 0u, get_headers::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == get_headers::id);
}

BOOST_AUTO_TEST_CASE(heading__headers_id__always__expected)
{
    const auto instance = heading{ 0u, headers::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == headers::id);
}

BOOST_AUTO_TEST_CASE(heading__inventory_id__always__expected)
{
    const auto instance = heading{ 0u, inventory::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == inventory::id);
}

BOOST_AUTO_TEST_CASE(heading__memory_pool_id__always__expected)
{
    const auto instance = heading{ 0u, memory_pool::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == memory_pool::id);
}

BOOST_AUTO_TEST_CASE(heading__merkle_block_id__always__expected)
{
    const auto instance = heading{ 0u, merkle_block::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == merkle_block::id);
}

BOOST_AUTO_TEST_CASE(heading__not_found_id__always__expected)
{
    const auto instance = heading{ 0u, not_found::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == not_found::id);
}

BOOST_AUTO_TEST_CASE(heading__ping_id__always__expected)
{
    const auto instance = heading{ 0u, ping::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == ping::id);
}

BOOST_AUTO_TEST_CASE(heading__pong_id__always__expected)
{
    const auto instance = heading{ 0u, pong::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == pong::id);
}

BOOST_AUTO_TEST_CASE(heading__reject_id__always__expected)
{
    const auto instance = heading{ 0u, reject::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == reject::id);
}

BOOST_AUTO_TEST_CASE(heading__send_compact_id__always__expected)
{
    const auto instance = heading{ 0u, send_compact::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == send_compact::id);
}

BOOST_AUTO_TEST_CASE(heading__send_headers_id__always__expected)
{
    const auto instance = heading{ 0u, send_headers::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == send_headers::id);
}

BOOST_AUTO_TEST_CASE(heading__transaction_id__always__expected)
{
    const auto instance = heading{ 0u, transaction::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == transaction::id);
}

BOOST_AUTO_TEST_CASE(heading__version_id__always__expected)
{
    const auto instance = heading{ 0u, version::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == version::id);
}

BOOST_AUTO_TEST_CASE(heading__version_acknowledge_id__always__expected)
{
    const auto instance = heading{ 0u, version_acknowledge::command, 0u, 0u };
    BOOST_REQUIRE(instance.id() == version_acknowledge::id);
}

BOOST_AUTO_TEST_CASE(heading__get_command__empty_payload__unknown)
{
    const system::data_chunk payload{};
    BOOST_REQUIRE_EQUAL(heading::get_command(payload), "<unknown>");
}

BOOST_AUTO_TEST_CASE(heading__get_command__short_payload__unknown)
{
    constexpr auto minimum = sizeof(uint32_t) + messages::heading::command_size;
    const system::data_chunk payload(sub1(minimum), 'a');
    BOOST_REQUIRE_EQUAL(heading::get_command(payload), "<unknown>");
}

BOOST_AUTO_TEST_CASE(heading__get_command__minimal_payload__expected)
{
    const system::data_chunk payload(
    {
        'a', 'b', 'c', 'd', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z'
    });

    BOOST_REQUIRE_EQUAL(heading::get_command(payload), "wxyzwxyzwxyz");
}

BOOST_AUTO_TEST_CASE(heading__get_command__extra_payload__expected)
{
    const system::data_chunk payload(
    {
        'a', 'b', 'c', 'd', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'A', 'B', 'C'
    });

    BOOST_REQUIRE_EQUAL(heading::get_command(payload), "wxyzwxyzwxyz");
}

// factory

BOOST_AUTO_TEST_CASE(heading__factory1__empty__expected)
{
    constexpr uint32_t magic = 42;
    constexpr auto command = "ping";
    const system::data_chunk payload{};
    const auto instance = heading::factory(magic, command, payload);

    BOOST_REQUIRE_EQUAL(instance.magic, magic);
    BOOST_REQUIRE_EQUAL(instance.command, command);
    BOOST_REQUIRE_EQUAL(instance.checksum, empty_checksum);
    BOOST_REQUIRE(instance.id() == identifier::ping);
}

BOOST_AUTO_TEST_CASE(heading__factory2__default_hash__expected)
{
    constexpr uint32_t magic = 42;
    constexpr auto command = "pong";
    const system::data_chunk payload{};
    const auto instance = heading::factory(magic, command, payload, {});

    BOOST_REQUIRE_EQUAL(instance.magic, magic);
    BOOST_REQUIRE_EQUAL(instance.command, command);
    BOOST_REQUIRE_EQUAL(instance.checksum, empty_checksum);
    BOOST_REQUIRE(instance.id() == identifier::pong);
}

BOOST_AUTO_TEST_CASE(heading__factory2__non_default_hash__expected)
{
    constexpr uint32_t magic = 42;
    constexpr auto command = "pong";
    const system::data_chunk payload{};
    const auto hash = system::to_shared(empty_hash);
    const auto instance = heading::factory(magic, command, payload, hash);

    BOOST_REQUIRE_EQUAL(instance.magic, magic);
    BOOST_REQUIRE_EQUAL(instance.command, command);
    BOOST_REQUIRE_EQUAL(instance.checksum, empty_checksum);
    BOOST_REQUIRE(instance.id() == identifier::pong);
}

BOOST_AUTO_TEST_SUITE_END()
