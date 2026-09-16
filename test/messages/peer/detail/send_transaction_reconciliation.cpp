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

BOOST_AUTO_TEST_SUITE(p2p_send_transaction_reconciliation_tests)

using namespace bc::system;
using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(send_transaction_reconciliation__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(send_transaction_reconciliation::command, "sendtxrcncl");
    constexpr auto index = messages::peer::registry::index_of<send_transaction_reconciliation>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), send_transaction_reconciliation::command);
    BOOST_REQUIRE_EQUAL(send_transaction_reconciliation::version_minimum, level::bip330);
    BOOST_REQUIRE_EQUAL(send_transaction_reconciliation::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(send_transaction_reconciliation__size__always__expected)
{
    constexpr auto expected = sizeof(uint32_t) + sizeof(uint64_t);
    BOOST_REQUIRE_EQUAL(send_transaction_reconciliation::size(level::bip330), expected);
}

BOOST_AUTO_TEST_CASE(send_transaction_reconciliation__deserialize__bip330__expected)
{
    constexpr auto payload = base16_array("01000000efcdab8967452301");
    system::read::bytes::copy source(payload);
    const auto message = send_transaction_reconciliation::deserialize(level::bip330, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(source.is_exhausted());
    BOOST_REQUIRE_EQUAL(message.value, 1u);
    BOOST_REQUIRE_EQUAL(message.salt, 0x0123456789abcdef_u64);
}

BOOST_AUTO_TEST_CASE(send_transaction_reconciliation__deserialize__insufficient_version__invalid)
{
    constexpr auto payload = base16_array("01000000efcdab8967452301");
    system::read::bytes::copy source(payload);
    send_transaction_reconciliation::deserialize(level::bip330 - 1u, source);
    BOOST_REQUIRE(!source);
}

BOOST_AUTO_TEST_CASE(send_transaction_reconciliation__serialize__always__round_trips)
{
    const send_transaction_reconciliation message{ 1u, 0x0123456789abcdef_u64 };
    data_chunk data{};
    system::write::bytes::data sink(data);
    message.serialize(level::bip330, sink);
    sink.flush();
    BOOST_REQUIRE(sink);
    BOOST_REQUIRE_EQUAL(data, base16_chunk("01000000efcdab8967452301"));
}

BOOST_AUTO_TEST_SUITE_END()
