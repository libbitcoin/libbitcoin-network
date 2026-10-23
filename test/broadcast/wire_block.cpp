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

BOOST_AUTO_TEST_SUITE(wire_block_tests)

static const system::chain::block& genesis() NOEXCEPT
{
    static const system::settings bitcoin{ system::chain::selection::mainnet };
    return bitcoin.genesis_block;
}

BOOST_AUTO_TEST_CASE(wire_block__command__always__block)
{
    BOOST_REQUIRE_EQUAL(std::string{ wire_block::command }, "block");
}

BOOST_AUTO_TEST_CASE(wire_block__block__shared__same_instance)
{
    const auto block = system::emplace_shared<const wire_block::view>(genesis().to_data(true), true);
    const wire_block instance{ block, true };
    BOOST_REQUIRE(instance.block() == block);
    BOOST_REQUIRE(instance.organized());
}

BOOST_AUTO_TEST_CASE(wire_block__organized__reorganized__false)
{
    const wire_block instance{ system::emplace_shared<const wire_block::view>(genesis().to_data(true), true), false };
    BOOST_REQUIRE(!instance.organized());
}

BOOST_AUTO_TEST_CASE(wire_block__transactions__genesis__coinbase_encoding)
{
    const wire_block instance{ system::emplace_shared<const wire_block::view>(genesis().to_data(true), true), true };
    BOOST_REQUIRE_EQUAL(instance.transactions().size(), 1u);
    BOOST_REQUIRE(instance.transactions().front()->tx()->to_data(true) == genesis().transactions_ptr()->front()->to_data(true));
}

BOOST_AUTO_TEST_CASE(wire_block__write__genesis__witness_encoding)
{
    const auto expected = genesis().to_data(true);
    const wire_block instance{ system::emplace_shared<const wire_block::view>(system::data_chunk{ expected }, true), true };
    BOOST_REQUIRE_EQUAL(instance.size(), expected.size());

    system::data_chunk out(instance.size());
    system::stream::out::fast sink{ out };
    system::write::bytes::fast writer{ sink };
    instance.write(writer);
    BOOST_REQUIRE(writer);
    BOOST_REQUIRE(out == expected);
}

BOOST_AUTO_TEST_SUITE_END()
