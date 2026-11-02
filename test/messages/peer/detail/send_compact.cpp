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

BC_AUTO_TEST_SUITE(p2p_send_compact_tests)

using namespace network::messages::peer;

BC_AUTO_TEST_CASE(send_compact__properties__always__expected)
{
    BC_REQUIRE_EQUAL(send_compact::command, "sendcmpct");
    constexpr auto index = messages::peer::registry::index_of<send_compact>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), send_compact::command);
    BC_REQUIRE_EQUAL(send_compact::version_minimum, level::bip152);
    BC_REQUIRE_EQUAL(send_compact::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(send_compact__size__always__expected)
{
    constexpr auto expected = sizeof(uint8_t)
        + sizeof(uint64_t);

    BC_REQUIRE_EQUAL(send_compact::size(level::canonical), expected);
}

BC_AUTO_TEST_CASE(send_compact__deserialize1__high_bandwidth_version1__expected)
{
    const auto data = system::base16_chunk("010100000000000000");
    const auto message = send_compact::deserialize(level::bip152, data);
    BC_REQUIRE(message);
    BC_REQUIRE(message->high_bandwidth);
    BC_REQUIRE_EQUAL(message->compact_version, 1u);
}

BC_AUTO_TEST_CASE(send_compact__deserialize1__low_bandwidth_version2__expected)
{
    const auto data = system::base16_chunk("000200000000000000");
    const auto message = send_compact::deserialize(level::bip152, data);
    BC_REQUIRE(message);
    BC_REQUIRE(!message->high_bandwidth);
    BC_REQUIRE_EQUAL(message->compact_version, send_compact::compact_version_2);
}

BC_AUTO_TEST_CASE(send_compact__deserialize1__non_boolean_mode__nullptr)
{
    const auto data = system::base16_chunk("020100000000000000");
    BC_REQUIRE(!send_compact::deserialize(level::bip152, data));
}

BC_AUTO_TEST_CASE(send_compact__deserialize1__insufficient_version__nullptr)
{
    const auto data = system::base16_chunk("010100000000000000");
    BC_REQUIRE(!send_compact::deserialize(level::bip152 - 1u, data));
}

BC_AUTO_TEST_CASE(send_compact__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("0101000000000000");
    BC_REQUIRE(!send_compact::deserialize(level::bip152, data));
}

BC_AUTO_TEST_CASE(send_compact__serialize1__high_bandwidth_version1__expected)
{
    const send_compact message{ true, 1u };
    system::data_chunk data(send_compact::size(level::bip152));
    BC_REQUIRE(message.serialize(level::bip152, data));
    BC_REQUIRE_EQUAL(data, system::base16_chunk("010100000000000000"));
}

BC_AUTO_TEST_CASE(send_compact__serialize1__low_bandwidth_version2__round_trips)
{
    const send_compact expected{ false, send_compact::compact_version_2 };
    system::data_chunk data(send_compact::size(level::bip152));
    BC_REQUIRE(expected.serialize(level::bip152, data));
    const auto message = send_compact::deserialize(level::bip152, data);
    BC_REQUIRE(message);
    BC_REQUIRE(!message->high_bandwidth);
    BC_REQUIRE_EQUAL(message->compact_version, expected.compact_version);
}

BC_AUTO_TEST_SUITE_END()
