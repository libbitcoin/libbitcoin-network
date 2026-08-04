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

BOOST_AUTO_TEST_SUITE(channel_tests)

// channel is abstract non-virtual base class.
struct accessor
  : public channel
{
    ////using channel::channel;
    accessor(const logger& log, const socket::ptr& socket, uint64_t identifier,
        const network::settings& settings,
        const channel::options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options)
    {
    }

    static uint32_t rate_limited1(const network::settings& settings,
        const channel::options_t& options) NOEXCEPT
    {
        return channel::rate_limited(settings, options);
    }
};

// rate_limited

BOOST_AUTO_TEST_CASE(channel__rate_limited__both_zero__zero)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 0;
    set.outbound.rate_limit = 0;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 0u);
}

BOOST_AUTO_TEST_CASE(channel__rate_limited__network_zero__service)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 0;
    set.outbound.rate_limit = 42;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 42u);
}

BOOST_AUTO_TEST_CASE(channel__rate_limited__service_zero__network)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 42;
    set.outbound.rate_limit = 0;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 42u);
}

BOOST_AUTO_TEST_CASE(channel__rate_limited__network_lesser__network)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 24;
    set.outbound.rate_limit = 42;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 24u);
}

BOOST_AUTO_TEST_CASE(channel__rate_limited__service_lesser__service)
{
    settings set(bc::system::chain::selection::mainnet);
    set.rate_limit = 42;
    set.outbound.rate_limit = 24;
    BOOST_REQUIRE_EQUAL(accessor::rate_limited1(set, set.outbound), 24u);
}

BOOST_AUTO_TEST_CASE(channel__stopped__default__false)
{
    constexpr auto expected = 42u;
    const logger log{};
    threadpool pool{ one };
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    network::socket::parameters params{ .maximum_request = 42 };
    auto socket_ptr = std::make_shared<network::socket>(log, pool.service(), std::move(params));
    auto channel_ptr = std::make_shared<accessor>(log, socket_ptr, expected, set, set.outbound);
    BOOST_REQUIRE(!channel_ptr->stopped());
    BOOST_REQUIRE_NE(channel_ptr->nonce(), zero);
    BOOST_REQUIRE_EQUAL(channel_ptr->identifier(), expected);

    // Stop completion is asynchronous.
    channel_ptr->stop(error::invalid_magic);
    channel_ptr.reset();
}

BOOST_AUTO_TEST_SUITE_END()
