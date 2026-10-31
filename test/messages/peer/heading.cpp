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

BC_AUTO_TEST_SUITE(p2p_heading_tests)

using namespace network::messages::peer;

// deserialize
// serialize

static_assert(heading::maximum_payload(true) == 4'000'000_size);
static_assert(heading::maximum_payload(false) == 1'800'003_size);

constexpr auto empty_hash = system::sha256::double_hash(system::sha256::ablocks_t<zero>{});
constexpr auto empty_checksum = system::from_little_endian<uint32_t>(empty_hash);

BC_AUTO_TEST_CASE(rpc_heading__size__always__expected)
{
    constexpr auto expected = sizeof(uint32_t)
        + heading::command_size
        + sizeof(uint32_t)
        + sizeof(uint32_t);

    BC_REQUIRE_EQUAL(heading::size(), expected);
}

BC_AUTO_TEST_CASE(rpc_heading__index__registered_commands__own_index)
{
    for (size_t expected{}; expected < messages::peer::registry::size; ++expected)
    {
        const std::string command{ messages::peer::registry::commands().at(expected) };
        const auto instance = heading{ 0u, command, 0u, 0u };
        BC_REQUIRE_EQUAL(instance.index(), expected);
    }
}

BC_AUTO_TEST_CASE(rpc_heading__index__unregistered_command__unknown)
{
    const auto instance = heading{ 0u, "bogus", 0u, 0u };
    BC_REQUIRE_EQUAL(instance.index(), messages::peer::registry::unknown);
}

BC_AUTO_TEST_CASE(rpc_heading__index__empty_command__unknown)
{
    const auto instance = heading{ 0u, "", 0u, 0u };
    BC_REQUIRE_EQUAL(instance.index(), messages::peer::registry::unknown);
}

BC_AUTO_TEST_CASE(rpc_heading__get_command__empty_payload__unknown)
{
    const system::data_chunk payload{};
    BC_REQUIRE_EQUAL(heading::get_command(payload), "<unknown>");
}

BC_AUTO_TEST_CASE(rpc_heading__get_command__short_payload__unknown)
{
    constexpr auto minimum = sizeof(uint32_t) + messages::peer::heading::command_size;
    const system::data_chunk payload(sub1(minimum), 'a');
    BC_REQUIRE_EQUAL(heading::get_command(payload), "<unknown>");
}

BC_AUTO_TEST_CASE(rpc_heading__get_command__minimal_payload__expected)
{
    const system::data_chunk payload(
    {
        'a', 'b', 'c', 'd', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z'
    });

    BC_REQUIRE_EQUAL(heading::get_command(payload), "wxyzwxyzwxyz");
}

BC_AUTO_TEST_CASE(rpc_heading__get_command__extra_payload__expected)
{
    const system::data_chunk payload(
    {
        'a', 'b', 'c', 'd', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'w', 'x', 'y', 'z', 'A', 'B', 'C'
    });

    BC_REQUIRE_EQUAL(heading::get_command(payload), "wxyzwxyzwxyz");
}

// factory

BC_AUTO_TEST_CASE(rpc_heading__factory1__empty__expected)
{
    constexpr uint32_t magic = 42;
    constexpr auto command = "ping";
    const system::data_chunk payload{};
    const auto instance = heading::factory(magic, command, payload);

    BC_REQUIRE_EQUAL(instance.magic, magic);
    BC_REQUIRE_EQUAL(instance.command, command);
    BC_REQUIRE_EQUAL(instance.checksum, empty_checksum);
    BC_REQUIRE_EQUAL(instance.index(), messages::peer::registry::index("ping"));
}

BC_AUTO_TEST_CASE(rpc_heading__factory2__default_hash__expected)
{
    constexpr uint32_t magic = 42;
    constexpr auto command = "pong";
    const system::data_chunk payload{};
    const auto instance = heading::factory(magic, command, payload);

    BC_REQUIRE_EQUAL(instance.magic, magic);
    BC_REQUIRE_EQUAL(instance.command, command);
    BC_REQUIRE_EQUAL(instance.checksum, empty_checksum);
    BC_REQUIRE_EQUAL(instance.index(), messages::peer::registry::index("pong"));
}

BC_AUTO_TEST_CASE(rpc_heading__factory2__empty_hash__expected)
{
    constexpr uint32_t magic = 42;
    constexpr auto command = "pong";
    const system::data_chunk payload{};
    const auto instance = heading::factory(magic, command, payload.size(), empty_hash);

    BC_REQUIRE_EQUAL(instance.magic, magic);
    BC_REQUIRE_EQUAL(instance.command, command);
    BC_REQUIRE_EQUAL(instance.checksum, empty_checksum);
    BC_REQUIRE_EQUAL(instance.index(), messages::peer::registry::index("pong"));
}

BC_AUTO_TEST_SUITE_END()
