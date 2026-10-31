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

BC_AUTO_TEST_SUITE(p2p_pong_tests)

using namespace bc::system;
using namespace network::messages::peer;

BC_AUTO_TEST_CASE(pong__properties__always__expected)
{
    BC_REQUIRE_EQUAL(pong::command, "pong");
    constexpr auto index = messages::peer::registry::index_of<pong>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), pong::command);
    BC_REQUIRE_EQUAL(pong::version_minimum, level::bip31);
    BC_REQUIRE_EQUAL(pong::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(pong__size__always__expected)
{
    constexpr auto expected = sizeof(uint64_t);
    BC_REQUIRE_EQUAL(pong::size(level::canonical), expected);
}

BC_AUTO_TEST_CASE(pong__deserialize1__bip31__expected)
{
    const auto data = system::base16_chunk("efcdab8967452301");
    const auto message = pong::deserialize(level::bip31, data);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->nonce, 0x0123456789abcdef_u64);
}

BC_AUTO_TEST_CASE(pong__deserialize1__insufficient_version__nullptr)
{
    const auto data = system::base16_chunk("efcdab8967452301");
    BC_REQUIRE(!pong::deserialize(level::bip31 - 1u, data));
}

BC_AUTO_TEST_CASE(pong__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("efcdab89674523");
    BC_REQUIRE(!pong::deserialize(level::bip31, data));
}

BC_AUTO_TEST_SUITE_END()
