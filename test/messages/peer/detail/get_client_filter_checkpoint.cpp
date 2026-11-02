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

BC_AUTO_TEST_SUITE(p2p_get_client_filter_checkpoint_tests)

using namespace network::messages::peer;

BC_AUTO_TEST_CASE(get_client_filter_checkpoint__properties__always__expected)
{
    BC_REQUIRE_EQUAL(get_client_filter_checkpoint::command, "getcfcheckpt");
    constexpr auto index = messages::peer::registry::index_of<get_client_filter_checkpoint>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), get_client_filter_checkpoint::command);
    BC_REQUIRE_EQUAL(get_client_filter_checkpoint::version_minimum, level::bip157);
    BC_REQUIRE_EQUAL(get_client_filter_checkpoint::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(get_client_filter_checkpoint__size__always__expected)
{
    constexpr auto expected = sizeof(uint8_t)
        + system::hash_size;

    BC_REQUIRE_EQUAL(get_client_filter_checkpoint::size(level::canonical), expected);
}

static const auto stop_hash = system::base16_hash("000000006c02c8ea6e4ff69651f7fcde348fb9d557a06e6957b65552002a7820");
static const auto payload = system::base16_chunk("00" "20782a005255b657696ea057d5b98f34defcf75196f64f6eeac8026c00000000");
static const uint32_t excess_version = add1<uint32_t>(level::maximum_protocol);

BC_AUTO_TEST_CASE(get_client_filter_checkpoint__deserialize1__valid__expected)
{
    const auto message = get_client_filter_checkpoint::deserialize(level::bip157, payload);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->filter_type, client_filter::type_id::neutrino);
    BC_REQUIRE_EQUAL(message->stop_hash, stop_hash);
}

BC_AUTO_TEST_CASE(get_client_filter_checkpoint__deserialize1__insufficient_version__nullptr)
{
    BC_REQUIRE(!get_client_filter_checkpoint::deserialize(level::bip152, payload));
}

BC_AUTO_TEST_CASE(get_client_filter_checkpoint__deserialize1__excess_version__nullptr)
{
    BC_REQUIRE(!get_client_filter_checkpoint::deserialize(excess_version, payload));
}

BC_AUTO_TEST_CASE(get_client_filter_checkpoint__deserialize1__underflow__nullptr)
{
    const system::data_chunk data{ payload.begin(), std::prev(payload.end()) };
    BC_REQUIRE(!get_client_filter_checkpoint::deserialize(level::bip157, data));
}

BC_AUTO_TEST_CASE(get_client_filter_checkpoint__deserialize2__valid__expected)
{
    system::read::bytes::copy source(payload);
    const auto message = get_client_filter_checkpoint::deserialize(level::maximum_protocol, source);
    BC_REQUIRE(source);
    BC_REQUIRE_EQUAL(message.stop_hash, stop_hash);
}

BC_AUTO_TEST_CASE(get_client_filter_checkpoint__serialize1__valid__expected)
{
    const get_client_filter_checkpoint message{ client_filter::type_id::neutrino, stop_hash };
    system::data_chunk data(get_client_filter_checkpoint::size(level::bip157));
    BC_REQUIRE(message.serialize(level::bip157, data));
    BC_REQUIRE_EQUAL(data, payload);
}

BC_AUTO_TEST_SUITE_END()
