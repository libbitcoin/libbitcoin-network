/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(channel_client_tests)

BOOST_AUTO_TEST_CASE(channel_client__stopped__default__false)
{
    constexpr auto expected_identifier = 42u;
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service());
    auto channel_ptr = std::make_shared<channel_client>(log, socket_ptr, set, expected_identifier);
    BOOST_REQUIRE(!channel_ptr->stopped());

    BOOST_REQUIRE_NE(channel_ptr->nonce(), zero);
    BOOST_REQUIRE_EQUAL(channel_ptr->identifier(), expected_identifier);

    // Stop completion is asynchronous.
    channel_ptr->stop(error::invalid_magic);
    channel_ptr.reset();
}

BOOST_AUTO_TEST_SUITE_END()
