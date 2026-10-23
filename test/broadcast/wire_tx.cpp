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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(wire_tx_tests)

static const system::chain::block& genesis() NOEXCEPT
{
    static const system::settings bitcoin{ system::chain::selection::mainnet };
    return bitcoin.genesis_block;
}

BOOST_AUTO_TEST_CASE(wire_tx__command__always__tx)
{
    BOOST_REQUIRE_EQUAL(std::string{ wire_tx::command }, "tx");
}

BOOST_AUTO_TEST_CASE(wire_tx__tx__shared__same_instance)
{
    const auto tx = system::emplace_shared<const wire_tx::view>(genesis().transactions_ptr()->front()->to_data(true), true, true);
    const wire_tx instance{ tx };
    BOOST_REQUIRE(instance.tx() == tx);
}

BOOST_AUTO_TEST_CASE(wire_tx__write__genesis_coinbase__witness_encoding)
{
    const auto expected = genesis().transactions_ptr()->front()->to_data(true);
    const wire_tx instance{ system::emplace_shared<const wire_tx::view>(system::data_chunk{ expected }, true, true) };
    BOOST_REQUIRE_EQUAL(instance.size(), expected.size());

    system::data_chunk out(instance.size());
    system::stream::out::fast sink{ out };
    system::write::bytes::fast writer{ sink };
    instance.write(writer);
    BOOST_REQUIRE(writer);
    BOOST_REQUIRE(out == expected);
}

BOOST_AUTO_TEST_SUITE_END()
