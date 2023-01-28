/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(acceptor_tests)

class accessor
  : public acceptor
{
public:
    using acceptor::acceptor;

    const settings& get_settings() const NOEXCEPT
    {
        return settings_;
    }

    const asio::io_context& get_service() const NOEXCEPT
    {
        return service_;
    }

    const asio::strand& get_strand() const NOEXCEPT
    {
        return strand_;
    }

    const asio::acceptor& get_acceptor() const NOEXCEPT
    {
        return acceptor_;
    }

    bool get_stopped() const NOEXCEPT
    {
        return stopped_;
    }
};

// TODO: increase test coverage.

BOOST_AUTO_TEST_CASE(acceptor__construct__default__stopped_expected)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), set);

    BOOST_REQUIRE(&instance->get_settings() == &set);
    BOOST_REQUIRE(&instance->get_service() == &pool.service());
    BOOST_REQUIRE(&instance->get_strand() == &strand);
    BOOST_REQUIRE(!instance->get_acceptor().is_open());
    BOOST_REQUIRE(instance->get_stopped());
    instance.reset();
}

// TODO: There is no way to fake failures in start.
BOOST_AUTO_TEST_CASE(acceptor__start__stop__success)
{
    const logger log{};
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), set);

    // Result codes inconsistent due to context.
    ////BOOST_REQUIRE_EQUAL(instance->start(42), error::success);
    instance->start(42);

    boost::asio::post(strand, [instance]()
    {
        instance->stop();
    });

    pool.stop();
    BOOST_REQUIRE(pool.join());

    BOOST_REQUIRE(instance->get_stopped());
    instance.reset();
}

BOOST_AUTO_TEST_CASE(acceptor__accept__stop__channel_stopped)
{
    // TODO: There is no way to fake successful acceptance.
    const logger log{};
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    settings set(bc::system::chain::selection::mainnet);
    auto instance = std::make_shared<accessor>(log, strand, pool.service(), set);

    // Result codes inconsistent due to context.
    ////BOOST_REQUIRE_EQUAL(instance->start(42), error::success);
    instance->start(42);

    boost::asio::post(strand, [instance]()
    {
        instance->accept([](const code&, const channel::ptr& channel)
        {
            // Result codes inconsistent due to context.
            ////BOOST_REQUIRE_EQUAL(ec, error::channel_stopped);
            BOOST_REQUIRE(!channel);
        });

        // Test race.
        std::this_thread::sleep_for(microseconds(1));
        instance->stop();
    });

    pool.stop();

    // This seems to imply that an assertion was hit.
    // terminate called after throwing an instance of 'std::length_error',  what():  basic_string::_M_create
    // unknown location(0): fatal error: in "acceptor_tests/acceptor__accept__stop__channel_stopped": signal: SIGABRT (application abort requested)
    // home/runner/work/libbitcoin-network/libbitcoin-network/test/net/acceptor.cpp(126): last checkpoint
    const auto joined = pool.join();
    BOOST_REQUIRE(joined);

    BOOST_REQUIRE(instance->get_stopped());
    instance.reset();
}

BOOST_AUTO_TEST_SUITE_END()
