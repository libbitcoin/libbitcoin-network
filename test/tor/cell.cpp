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

BC_AUTO_TEST_SUITE(tor_cell_tests)

using namespace bc::system;
using namespace tor;

const auto versions = base16_chunk("00040005");
const auto versions_cell = base16_chunk("00000700040004" "0005");
const auto netinfo = base16_chunk("0000000004047f00000100");

BC_AUTO_TEST_CASE(tor_cell__is_variable__commands__expected)
{
    BC_REQUIRE(is_variable(command::versions));
    BC_REQUIRE(is_variable(command::vpadding));
    BC_REQUIRE(is_variable(command::certs));
    BC_REQUIRE(is_variable(command::auth_challenge));
    BC_REQUIRE(!is_variable(command::padding));
    BC_REQUIRE(!is_variable(command::netinfo));
    BC_REQUIRE(!is_variable(command::create2));
}

BC_AUTO_TEST_CASE(tor_cell__encode__narrow_versions__expected)
{
    BC_REQUIRE_EQUAL(encode({ 0, command::versions, versions }, true), versions_cell);
}

BC_AUTO_TEST_CASE(tor_cell__encode__fixed__zero_padded)
{
    const auto encoded = encode({ 0x01020304, command::netinfo, netinfo });
    const auto expected = splice(base16_chunk("0102030408"), netinfo);
    const data_chunk head{ encoded.begin(), std::next(encoded.begin(), expected.size()) };
    const data_chunk tail{ std::next(encoded.begin(), expected.size()), encoded.end() };
    BC_REQUIRE_EQUAL(encoded.size(), circuit_size + 1u + payload_size);
    BC_REQUIRE_EQUAL(head, expected);
    BC_REQUIRE_EQUAL(tail, data_chunk(payload_size - netinfo.size()));
}

BC_AUTO_TEST_CASE(tor_cell__encode__variable__length_prefixed)
{
    BC_REQUIRE_EQUAL(encode({ 0, command::vpadding, base16_chunk("abcd") }), base16_chunk("0000000080" "0002" "abcd"));
}

BC_AUTO_TEST_CASE(tor_cell__decode__narrow_versions__expected)
{
    cell out{};
    BC_REQUIRE_EQUAL(decode(out, versions_cell, true), versions_cell.size());
    BC_REQUIRE_EQUAL(out.circuit, 0u);
    BC_REQUIRE_EQUAL(out.command, command::versions);
    BC_REQUIRE_EQUAL(out.payload, versions);
}

BC_AUTO_TEST_CASE(tor_cell__decode__fixed__round_trip)
{
    const auto encoded = encode({ 42, command::netinfo, netinfo });
    cell out{};
    BC_REQUIRE_EQUAL(decode(out, encoded), encoded.size());
    BC_REQUIRE_EQUAL(out.circuit, 42u);
    BC_REQUIRE_EQUAL(out.command, command::netinfo);
    BC_REQUIRE_EQUAL(out.payload.size(), payload_size);
}

BC_AUTO_TEST_CASE(tor_cell__decode__variable_followed__first_cell_size)
{
    const auto first = encode({ 0, command::vpadding, base16_chunk("abcd") });
    const auto data = splice(first, encode({ 0, command::padding, {} }));
    cell out{};
    BC_REQUIRE_EQUAL(decode(out, data), first.size());
    BC_REQUIRE_EQUAL(out.payload, base16_chunk("abcd"));
}

BC_AUTO_TEST_CASE(tor_cell__decode__incomplete_fixed__zero)
{
    auto encoded = encode({ 0, command::netinfo, netinfo });
    encoded.pop_back();
    cell out{};
    BC_REQUIRE_EQUAL(decode(out, encoded), 0u);
}

BC_AUTO_TEST_CASE(tor_cell__decode__incomplete_variable__zero)
{
    cell out{};
    BC_REQUIRE_EQUAL(decode(out, base16_chunk("0000000080" "0002" "ab")), 0u);
}

BC_AUTO_TEST_CASE(tor_cell__decode__incomplete_header__zero)
{
    cell out{};
    BC_REQUIRE_EQUAL(decode(out, base16_chunk("000000")), 0u);
}

BC_AUTO_TEST_SUITE_END()
