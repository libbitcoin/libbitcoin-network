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

BOOST_AUTO_TEST_SUITE(p2p_ping_tests)

using namespace bc::system;
using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(ping__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(ping::command, "ping");
    constexpr auto index = messages::peer::registry::index_of<ping>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), ping::command);
    BOOST_REQUIRE_EQUAL(ping::version_minimum, level::minimum_protocol);
    BOOST_REQUIRE_EQUAL(ping::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(ping__size__always__expected)
{
    // bip31 added nonce field
    BOOST_REQUIRE_EQUAL(ping::size(level::canonical), zero);
    BOOST_REQUIRE_EQUAL(ping::size(level::bip31), sizeof(uint64_t));
}

BOOST_AUTO_TEST_CASE(ping__deserialize1__bip31__expected)
{
    const auto data = system::base16_chunk("efcdab8967452301");
    const auto message = ping::deserialize(level::bip31, data);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->nonce, 0x0123456789abcdef_u64);
}

BOOST_AUTO_TEST_CASE(ping__deserialize1__pre_bip31_empty__zero_nonce)
{
    const system::data_chunk data{};
    const auto message = ping::deserialize(level::minimum_protocol, data);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->nonce, 0u);
}

BOOST_AUTO_TEST_CASE(ping__deserialize1__insufficient_version__nullptr)
{
    const system::data_chunk data{};
    BOOST_REQUIRE(!ping::deserialize(level::minimum_protocol - 1u, data));
}

BOOST_AUTO_TEST_CASE(ping__deserialize1__excessive_version__nullptr)
{
    const auto data = system::base16_chunk("efcdab8967452301");
    BOOST_REQUIRE(!ping::deserialize(level::maximum_protocol + 1u, data));
}

BOOST_AUTO_TEST_SUITE_END()
