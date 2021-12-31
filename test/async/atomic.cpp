/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network.hpp>

BOOST_AUTO_TEST_SUITE(atomic_tests)

struct foo
{
    bool bar;
};

// default values

BOOST_AUTO_TEST_CASE(atomic__integral__default__false)
{
    atomic<bool> instance{};
    BOOST_REQUIRE(!instance.load());
}

BOOST_AUTO_TEST_CASE(atomic__struct__default__false)
{
    atomic<foo> instance{};
    BOOST_REQUIRE(!instance.load().bar);
}

// integral value

BOOST_AUTO_TEST_CASE(atomic__integral_load__false__false)
{
    atomic<bool> instance(false);
    BOOST_REQUIRE(!instance.load());
}

BOOST_AUTO_TEST_CASE(atomic__integral_load__true__true)
{
    atomic<bool> instance(true);
    BOOST_REQUIRE(instance.load());
}

BOOST_AUTO_TEST_CASE(atomic__integral_store__true__true)
{
    atomic<bool> instance{};
    instance.store(true);
    BOOST_REQUIRE(instance.load());
}

BOOST_AUTO_TEST_CASE(atomic__integral_store__true_false__false)
{
    atomic<bool> instance{};
    instance.store(true);
    instance.store(false);
    BOOST_REQUIRE(!instance.load());
}

// references

BOOST_AUTO_TEST_CASE(atomic__reference_load__false__false)
{
    const foo value{ false };
    atomic<foo> instance(value);
    BOOST_REQUIRE(!instance.load().bar);
}

BOOST_AUTO_TEST_CASE(atomic__reference_load__true__true)
{
    const foo value{ true };
    atomic<foo> instance(value);
    BOOST_REQUIRE(instance.load().bar);
}

BOOST_AUTO_TEST_CASE(atomic__reference_store__true__true)
{
    const foo value{ true };
    atomic<foo> instance{};
    instance.store(value);
    BOOST_REQUIRE(instance.load().bar);
}

BOOST_AUTO_TEST_CASE(atomic__reference_store__true_false__true)
{
    foo value{ true };
    atomic<foo> instance{};
    instance.store(value);

    // Since decay converts to a copy, this should not affect the atomic.
    value.bar = false;
    BOOST_REQUIRE(instance.load().bar);
}

// moves

BOOST_AUTO_TEST_CASE(atomic__move_load__false__false)
{
    atomic<foo> instance(foo{ false });
    BOOST_REQUIRE(!instance.load().bar);
}

BOOST_AUTO_TEST_CASE(atomic__move_load__true__true)
{
    atomic<foo> instance(foo{ true });
    BOOST_REQUIRE(instance.load().bar);
}

BOOST_AUTO_TEST_CASE(atomic__move_store__true__true)
{
    atomic<foo> instance{};
    instance.store(foo{ true });
    BOOST_REQUIRE(instance.load().bar);
}

BOOST_AUTO_TEST_CASE(atomic__move_store__true_false__false)
{
    network::atomic<foo> instance{};
    instance.store(foo{ true });
    instance.store(foo{ false });
    BOOST_REQUIRE(!instance.load().bar);
}

// mixed

BOOST_AUTO_TEST_CASE(atomic__move_store__move_false_reference_true__true)
{
    const foo value{ true };
    atomic<foo> instance(foo{ false });
    instance.store(value);
    BOOST_REQUIRE(instance.load().bar);
}

BOOST_AUTO_TEST_CASE(atomic__move_store__reference_false_move_true__true)
{
    const foo value{ false };
    atomic<foo> instance(value);
    instance.store(foo{ true });
    BOOST_REQUIRE(instance.load().bar);
}

BOOST_AUTO_TEST_SUITE_END()
