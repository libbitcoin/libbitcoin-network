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

BOOST_AUTO_TEST_SUITE(connector_tests)

class accessor
  : public connector
{
public:
    using connector::connector;

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
};

BOOST_AUTO_TEST_CASE(connector__construct__default__expected_properties)
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

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_CASE(connector__connect1__timeout__channel_timeout)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    settings set(bc::system::chain::selection::mainnet);
    set.connect_timeout_seconds = 0;
    auto instance = std::make_shared<connector>(strand, pool.service(), set);

    boost::asio::post(strand, [&]()
    {
        instance->connect(config::endpoint{ "bogus.xxx", 42 }, [](const code& ec, channel::ptr channel)
        {
            BOOST_REQUIRE_EQUAL(ec, error::channel_timeout);
            BOOST_REQUIRE(!channel);
        });
    });

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_CASE(connector__connect2__timeout__channel_timeout)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    settings set(bc::system::chain::selection::mainnet);
    set.connect_timeout_seconds = 0;
    auto instance = std::make_shared<connector>(strand, pool.service(), set);

    boost::asio::post(strand, [&]()
    {
        instance->connect(config::authority{ "42.42.42.42", 42 }, [](const code& ec, channel::ptr channel)
        {
            BOOST_REQUIRE_EQUAL(ec, error::channel_timeout);
            BOOST_REQUIRE(!channel);
        });
    });

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_CASE(connector__connect3__timeout__channel_timeout)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    settings set(bc::system::chain::selection::mainnet);
    set.connect_timeout_seconds = 0;
    auto instance = std::make_shared<connector>(strand, pool.service(), set);

    boost::asio::post(strand, [&]()
    {
        instance->connect("bogus.xxx", 42, [](const code& ec, channel::ptr channel)
        {
            BOOST_REQUIRE_EQUAL(ec, error::channel_timeout);
            BOOST_REQUIRE(!channel);
        });
    });

    pool.stop();
    pool.join();
}

BOOST_AUTO_TEST_CASE(connector__connect__stop__operation_canceled)
{
    threadpool pool(2);
    asio::strand strand(pool.service().get_executor());
    settings set(bc::system::chain::selection::mainnet);
    set.connect_timeout_seconds = 1000;
    auto instance = std::make_shared<connector>(strand, pool.service(), set);

    boost::asio::post(strand, [&]()
    {
        instance->connect(config::endpoint{ "bogus.xxx", 42 }, [](const code& ec, channel::ptr channel)
        {
            // TODO: 11001 (HOST_NOT_FOUND) gets mapped to unknown.
            BOOST_REQUIRE(ec == error::unknown || ec == error::operation_canceled);
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
