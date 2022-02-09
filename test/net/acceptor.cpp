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

    const settings& get_settings() const
    {
        return settings_;
    }

    const asio::io_context& get_service() const
    {
        return service_;
    }

    const asio::strand& get_strand() const
    {
        return strand_;
    }

    bool get_stopped() const
    {
        return stopped_;
    }

    deadline::ptr get_timer()
    {
        return timer_;
    }

    asio::acceptor& get_acceptor()
    {
        return acceptor_;
    }
};

BOOST_AUTO_TEST_CASE(acceptor__construct__default__expected_properties)
{
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto instance = std::make_shared<accessor>(strand, pool.service(), set);

    BOOST_REQUIRE(&instance->get_settings() == &set);
    BOOST_REQUIRE(&instance->get_service() == &pool.service());
    BOOST_REQUIRE(&instance->get_strand() == &strand);
    BOOST_REQUIRE(instance->get_stopped());
    BOOST_REQUIRE(instance->get_timer());
    BOOST_REQUIRE(!instance->get_acceptor().is_open());

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_CASE(acceptor__start__always__expected)
{
    threadpool pool(1);
    asio::strand strand(pool.service().get_executor());
    const settings set(bc::system::chain::selection::mainnet);
    auto instance = std::make_shared<acceptor>(strand, pool.service(), set);
    instance->start(42);
    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_CASE(acceptor__accept__timeout__channel_timeout)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    settings set(bc::system::chain::selection::mainnet);
    set.connect_timeout_seconds = 0;
    auto instance = std::make_shared<acceptor>(strand, pool.service(), set);
    BOOST_REQUIRE(!instance->start(42));

    boost::asio::post(strand, [&]()
    {
        instance->accept([](const code& ec, channel::ptr channel)
        {
            BOOST_REQUIRE_EQUAL(ec, error::channel_timeout);
            BOOST_REQUIRE(!channel);
        });
    });

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_CASE(acceptor__accept__stop__operation_canceled)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    settings set(bc::system::chain::selection::mainnet);
    set.connect_timeout_seconds = 1000;
    auto instance = std::make_shared<acceptor>(strand, pool.service(), set);
    BOOST_REQUIRE(!instance->start(42));

    boost::asio::post(strand, [&]()
    {
        instance->accept([](const code& ec, channel::ptr channel)
        {
            BOOST_REQUIRE_EQUAL(ec, error::operation_canceled);
            BOOST_REQUIRE(!channel);
        });

        // Test race.
        std::this_thread::sleep_for(microseconds(1));
        instance->stop();
    });

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_SUITE_END()
