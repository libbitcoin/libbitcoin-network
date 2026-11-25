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
#include "../../test.hpp"

BC_AUTO_TEST_SUITE(sv2_registry_tests)

using namespace bc::system;
using namespace network::messages::sv2;

static_assert(registry::size == 26_size);
static_assert(registry::unknown == registry::size);
static_assert(registry::index_of<setup_connection>() == 0_size);
static_assert(registry::index_of<heading>() == registry::unknown);
static_assert(registry::index(0x00_u8) == registry::index_of<setup_connection>());
static_assert(registry::index(0x21_u8) == registry::index_of<set_target>());
static_assert(registry::index_of<setup_connection_error>() == 2_size);
static_assert(registry::index_of<channel_endpoint_changed>() == 3_size);
static_assert(registry::index(0x1e_u8) == registry::unknown);
static_assert(registry::index("set_target") == registry::index_of<set_target>());
static_assert(registry::index("bogus") == registry::unknown);

constexpr auto target_index = registry::index_of<set_target>();

// A set_target payload and its frame.
static data_chunk target_payload() NOEXCEPT
{
    return base16_chunk("01030202202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
}

static data_chunk target_frame() NOEXCEPT
{
    return base16_chunk("00802124000001030202202122232425262728292a2b2c2d2e2f303132333435363738393a3b3c3d3e3f");
}

BC_AUTO_TEST_CASE(sv2_registry__to_any__registered__message)
{
    const auto message = registry::to_any(target_index, target_payload()).get<const set_target>();
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->channel_id, 0x02020301u);
}

BC_AUTO_TEST_CASE(sv2_registry__to_any__other_message_type__not_held)
{
    BC_REQUIRE(!registry::to_any(target_index, target_payload()).get<const close_channel>());
}

BC_AUTO_TEST_CASE(sv2_registry__to_any__truncated__empty)
{
    BC_REQUIRE(!registry::to_any(target_index, base16_chunk("01030202")));
}

BC_AUTO_TEST_CASE(sv2_registry__to_any__unknown__empty)
{
    BC_REQUIRE(!registry::to_any(registry::unknown, target_payload()));
}

BC_AUTO_TEST_CASE(sv2_registry__to_frame__registered__expected)
{
    const auto data = registry::to_frame(target_index, registry::to_any(target_index, target_payload()));
    BC_REQUIRE(data);
    BC_REQUIRE_EQUAL(*data, target_frame());
}

BC_AUTO_TEST_CASE(sv2_registry__to_frame__other_message_index__nullptr)
{
    BC_REQUIRE(!registry::to_frame(registry::index_of<close_channel>(), registry::to_any(target_index, target_payload())));
}

BC_AUTO_TEST_CASE(sv2_registry__to_frame__unknown__nullptr)
{
    BC_REQUIRE(!registry::to_frame(registry::unknown, registry::to_any(target_index, target_payload())));
}

BC_AUTO_TEST_CASE(sv2_registry__to_frame__sized_data__expected)
{
    data_chunk data(target_frame().size());
    BC_REQUIRE(registry::to_frame(target_index, registry::to_any(target_index, target_payload()), data_slab{ data }));
    BC_REQUIRE_EQUAL(data, target_frame());
}

BC_AUTO_TEST_CASE(sv2_registry__to_frame__data_below_heading__false)
{
    data_chunk data(sub1(heading::size()));
    BC_REQUIRE(!registry::to_frame(target_index, registry::to_any(target_index, target_payload()), data_slab{ data }));
}

BC_AUTO_TEST_SUITE_END()
