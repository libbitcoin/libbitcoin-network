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

BC_AUTO_TEST_SUITE(p2p_send_transaction_reconciliation_tests)

using namespace bc::system;
using namespace network::messages::peer;

BC_AUTO_TEST_CASE(send_transaction_reconciliation__properties__always__expected)
{
    BC_REQUIRE_EQUAL(send_transaction_reconciliation::command, "sendtxrcncl");
    constexpr auto index = messages::peer::registry::index_of<send_transaction_reconciliation>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), send_transaction_reconciliation::command);
    BC_REQUIRE_EQUAL(send_transaction_reconciliation::version_minimum, level::bip330);
    BC_REQUIRE_EQUAL(send_transaction_reconciliation::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(send_transaction_reconciliation__size__always__expected)
{
    constexpr auto expected = sizeof(uint32_t) + sizeof(uint64_t);
    BC_REQUIRE_EQUAL(send_transaction_reconciliation::size(level::bip330), expected);
}

BC_AUTO_TEST_CASE(send_transaction_reconciliation__deserialize__bip330__expected)
{
    constexpr auto payload = base16_array("01000000efcdab8967452301");
    system::read::bytes::copy source(payload);
    const auto message = send_transaction_reconciliation::deserialize(level::bip330, source);
    BC_REQUIRE(source);
    BC_REQUIRE(source.is_exhausted());
    BC_REQUIRE_EQUAL(message.value, 1u);
    BC_REQUIRE_EQUAL(message.salt, 0x0123456789abcdef_u64);
}

BC_AUTO_TEST_CASE(send_transaction_reconciliation__deserialize__insufficient_version__invalid)
{
    constexpr auto payload = base16_array("01000000efcdab8967452301");
    system::read::bytes::copy source(payload);
    send_transaction_reconciliation::deserialize(level::bip330 - 1u, source);
    BC_REQUIRE(!source);
}

BC_AUTO_TEST_CASE(send_transaction_reconciliation__serialize__always__round_trips)
{
    const send_transaction_reconciliation message{ 1u, 0x0123456789abcdef_u64 };
    data_chunk data{};
    system::write::bytes::data sink(data);
    message.serialize(level::bip330, sink);
    sink.flush();
    BC_REQUIRE(sink);
    BC_REQUIRE_EQUAL(data, base16_chunk("01000000efcdab8967452301"));
}

BC_AUTO_TEST_CASE(send_transaction_reconciliation__deserialize1__bip330__expected)
{
    const auto data = base16_chunk("01000000efcdab8967452301");
    const auto message = send_transaction_reconciliation::deserialize(level::bip330, data);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->value, 1u);
    BC_REQUIRE_EQUAL(message->salt, 0x0123456789abcdef_u64);
}

BC_AUTO_TEST_CASE(send_transaction_reconciliation__deserialize1__insufficient_version__nullptr)
{
    const auto data = base16_chunk("01000000efcdab8967452301");
    BC_REQUIRE(!send_transaction_reconciliation::deserialize(level::bip330 - 1u, data));
}

BC_AUTO_TEST_CASE(send_transaction_reconciliation__deserialize1__underflow__nullptr)
{
    const auto data = base16_chunk("01000000efcdab89674523");
    BC_REQUIRE(!send_transaction_reconciliation::deserialize(level::bip330, data));
}

BC_AUTO_TEST_CASE(send_transaction_reconciliation__serialize1__always__expected)
{
    const send_transaction_reconciliation message{ 1u, 0x0123456789abcdef_u64 };
    data_chunk data(send_transaction_reconciliation::size(level::bip330));
    BC_REQUIRE(message.serialize(level::bip330, data));
    BC_REQUIRE_EQUAL(data, base16_chunk("01000000efcdab8967452301"));
}

BC_AUTO_TEST_CASE(send_transaction_reconciliation__serialize1__overflow__false)
{
    const send_transaction_reconciliation message{ 1u, 0x0123456789abcdef_u64 };
    data_chunk data(sub1(send_transaction_reconciliation::size(level::bip330)));
    BC_REQUIRE(!message.serialize(level::bip330, data));
}

BC_AUTO_TEST_SUITE_END()
