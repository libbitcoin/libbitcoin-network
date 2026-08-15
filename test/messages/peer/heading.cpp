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

BOOST_AUTO_TEST_SUITE(p2p_heading_tests)

using namespace network::messages::peer;

// deserialize
// serialize

static_assert(heading::maximum_payload(0, true) == 4'000'000_size);
static_assert(heading::maximum_payload(0, false) == 1'800'003_size);
static_assert(heading::maximum_payload(max_uint32, true) == 4'000'000_size);
static_assert(heading::maximum_payload(max_uint32, false) == 1'800'003_size);

constexpr auto empty_hash = system::sha256::double_hash(system::sha256::ablocks_t<zero>{});
constexpr auto empty_checksum = system::from_little_endian<uint32_t>(empty_hash);

BOOST_AUTO_TEST_CASE(rpc_heading__size__always__expected)
{
    constexpr auto expected = sizeof(uint32_t)
        + heading::command_size
        + sizeof(uint32_t)
        + sizeof(uint32_t);

    BOOST_REQUIRE_EQUAL(heading::size(), expected);
}

BOOST_AUTO_TEST_CASE(rpc_heading__index__registered_commands__own_index)
{
    for (size_t expected{}; expected < rpc::peer_registry::size; ++expected)
    {
        const std::string command{ rpc::peer_registry::commands().at(expected) };
        const auto instance = heading{ 0u, command, 0u, 0u };
        BOOST_REQUIRE_EQUAL(instance.index(), expected);
    }
}

BOOST_AUTO_TEST_CASE(rpc_heading__index__unregistered_command__unknown)
{
    const auto instance = heading{ 0u, "bogus", 0u, 0u };
    BOOST_REQUIRE_EQUAL(instance.index(), rpc::peer_registry::unknown);
}

BOOST_AUTO_TEST_CASE(rpc_heading__index__empty_command__unknown)
{
    const auto instance = heading{ 0u, "", 0u, 0u };
    BOOST_REQUIRE_EQUAL(instance.index(), rpc::peer_registry::unknown);
}

BOOST_AUTO_TEST_CASE(rpc_heading__get_command__empty_payload__unknown)
{
    const system::data_chunk payload{};
    BOOST_REQUIRE_EQUAL(heading::get_command(payload), "<unknown>");
}

BOOST_AUTO_TEST_CASE(rpc_heading__get_command__short_payload__unknown)
{
    constexpr auto minimum = sizeof(uint32_t) + messages::peer::heading::command_size;
    const system::data_chunk payload(sub1(minimum), 'a');
    BOOST_REQUIRE_EQUAL(heading::get_command(payload), "<unknown>");
}

BOOST_AUTO_TEST_CASE(rpc_heading__get_command__minimal_payload__expected)
{
    const system::data_chunk payload(
    {
        'a', 'b', 'c', 'd', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z'
    });

    BOOST_REQUIRE_EQUAL(heading::get_command(payload), "wxyzwxyzwxyz");
}

BOOST_AUTO_TEST_CASE(rpc_heading__get_command__extra_payload__expected)
{
    const system::data_chunk payload(
    {
        'a', 'b', 'c', 'd', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'A', 'B', 'C'
    });

    BOOST_REQUIRE_EQUAL(heading::get_command(payload), "wxyzwxyzwxyz");
}

// factory

BOOST_AUTO_TEST_CASE(rpc_heading__factory1__empty__expected)
{
    constexpr uint32_t magic = 42;
    constexpr auto command = "ping";
    const system::data_chunk payload{};
    const auto instance = heading::factory(magic, command, payload);

    BOOST_REQUIRE_EQUAL(instance.magic, magic);
    BOOST_REQUIRE_EQUAL(instance.command, command);
    BOOST_REQUIRE_EQUAL(instance.checksum, empty_checksum);
    BOOST_REQUIRE_EQUAL(instance.index(), rpc::peer_registry::index("ping"));
}

BOOST_AUTO_TEST_CASE(rpc_heading__factory2__default_hash__expected)
{
    constexpr uint32_t magic = 42;
    constexpr auto command = "pong";
    const system::data_chunk payload{};
    const auto instance = heading::factory(magic, command, payload);

    BOOST_REQUIRE_EQUAL(instance.magic, magic);
    BOOST_REQUIRE_EQUAL(instance.command, command);
    BOOST_REQUIRE_EQUAL(instance.checksum, empty_checksum);
    BOOST_REQUIRE_EQUAL(instance.index(), rpc::peer_registry::index("pong"));
}

BOOST_AUTO_TEST_CASE(rpc_heading__factory2__empty_hash__expected)
{
    constexpr uint32_t magic = 42;
    constexpr auto command = "pong";
    const system::data_chunk payload{};
    const auto instance = heading::factory(magic, command, payload.size(), empty_hash);

    BOOST_REQUIRE_EQUAL(instance.magic, magic);
    BOOST_REQUIRE_EQUAL(instance.command, command);
    BOOST_REQUIRE_EQUAL(instance.checksum, empty_checksum);
    BOOST_REQUIRE_EQUAL(instance.index(), rpc::peer_registry::index("pong"));
}

BOOST_AUTO_TEST_SUITE_END()
