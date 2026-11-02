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

BC_AUTO_TEST_SUITE(p2p_fee_filter_tests)

using namespace network::messages::peer;

BC_AUTO_TEST_CASE(fee_filter__properties__always__expected)
{
    BC_REQUIRE_EQUAL(fee_filter::command, "feefilter");
    constexpr auto index = messages::peer::registry::index_of<fee_filter>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), fee_filter::command);
    BC_REQUIRE_EQUAL(fee_filter::version_minimum, level::bip133);
    BC_REQUIRE_EQUAL(fee_filter::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(fee_filter__size__always__expected)
{
    constexpr auto expected = sizeof(uint64_t);
    BC_REQUIRE_EQUAL(fee_filter::size(level::canonical), expected);
}

static const auto payload = system::base16_chunk("e803000000000000");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BC_AUTO_TEST_CASE(fee_filter__deserialize1__valid__expected)
{
    const auto message = fee_filter::deserialize(level::bip133, payload);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->minimum_fee, 1000u);
}

BC_AUTO_TEST_CASE(fee_filter__deserialize1__insufficient_version__nullptr)
{
    BC_REQUIRE(!fee_filter::deserialize(level::bip130, payload));
}

BC_AUTO_TEST_CASE(fee_filter__deserialize1__excess_version__nullptr)
{
    BC_REQUIRE(!fee_filter::deserialize(excess_version, payload));
}

BC_AUTO_TEST_CASE(fee_filter__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("e8030000000000");
    BC_REQUIRE(!fee_filter::deserialize(level::bip133, data));
}

BC_AUTO_TEST_CASE(fee_filter__deserialize2__valid__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = fee_filter::deserialize(level::maximum_protocol, source);
    BC_REQUIRE(source);
    BC_REQUIRE_EQUAL(message.minimum_fee, 1000u);
}

BC_AUTO_TEST_CASE(fee_filter__serialize1__valid__expected)
{
    const fee_filter message{ 1000 };
    system::data_chunk data(fee_filter::size(level::bip133));
    BC_REQUIRE(message.serialize(level::bip133, data));
    BC_REQUIRE_EQUAL(data, payload);
}

BC_AUTO_TEST_SUITE_END()
