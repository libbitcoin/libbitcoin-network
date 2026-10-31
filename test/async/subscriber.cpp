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

BC_AUTO_TEST_SUITE(subscriber_tests)

typedef subscriber<size_t> test_subscriber;

BC_AUTO_TEST_CASE(subscriber__subscribe__stopped__subscriber_stopped)
{
    test_subscriber instance{};
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    std::pair<code, size_t> stop_result;
    std::pair<code, size_t> retry_result;

    result &= instance.empty();
    result &= is_zero(instance.size());
    result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
    {
        stop_result = { value, size };
    });

    result &= !instance.empty();
    result &= is_one(instance.size());
    instance.stop(ec, expected);

    result &= instance.empty();
    result &= is_zero(instance.size());
    result &= (instance.subscribe([&](code value, size_t size) NOEXCEPT
    {
        retry_result = { value, size };
    }) == error::subscriber_stopped);

    result &= is_zero(instance.size());
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(stop_result.first, ec);
    BC_REQUIRE_EQUAL(stop_result.second, expected);
    BC_REQUIRE_EQUAL(retry_result.first, error::subscriber_stopped);
    BC_REQUIRE_EQUAL(retry_result.second, size_t{});
}

BC_AUTO_TEST_CASE(subscriber__stop_default__once__expected)
{
    test_subscriber instance{};
    const auto ec = error::address_not_found;
    constexpr auto expected = zero;

    auto result = true;
    std::pair<code, size_t> stop_result;

    result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
    {
        stop_result = { value, size };
    });

    instance.stop_default(ec);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(stop_result.first, ec);
    BC_REQUIRE_EQUAL(stop_result.second, expected);
}

BC_AUTO_TEST_CASE(subscriber__stop__once__expected)
{
    test_subscriber instance{};
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;
    
    auto result = true;
    std::pair<code, size_t> stop_result;

    result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
    {
        stop_result = { value, size };
    });

    instance.stop(ec, expected);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(stop_result.first, ec);
    BC_REQUIRE_EQUAL(stop_result.second, expected);
}

BC_AUTO_TEST_CASE(subscriber__stop__twice__second_dropped)
{
    test_subscriber instance{};
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto result = true;
    std::pair<code, size_t> stop_result;

    result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
    {
        stop_result = { value, size };
    });

    instance.stop(ec, expected);
    instance.stop(error::address_blocked, {});

    // Handler is not invoked on stop after stop (handlers cleared).
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(stop_result.first, ec);
    BC_REQUIRE_EQUAL(stop_result.second, expected);
}

BC_AUTO_TEST_CASE(subscriber__notify__stopped__dropped)
{
    test_subscriber instance{};
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto count = zero;
    auto result = true;
    std::pair<code, size_t> notify_result;

    result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
    {
        // Allow first and possible second notify, ignore stop.
        if (++count != two) notify_result = { value, size };
    });

    instance.notify(ec, expected);
    instance.stop_default(error::address_blocked);
    instance.notify(error::address_blocked, {});
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(notify_result.first, ec);
    BC_REQUIRE_EQUAL(notify_result.second, expected);
}

BC_AUTO_TEST_CASE(subscriber__notify__once__expected)
{
    test_subscriber instance{};
    const auto ec = error::address_not_found;
    constexpr auto expected = 42u;

    auto count = zero;
    auto result = true;
    std::pair<code, size_t> notify_result;

    result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
    {
        if (is_one(++count)) notify_result = { value, size };
    });

    instance.notify(ec, expected);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(notify_result.first, ec);
    BC_REQUIRE_EQUAL(notify_result.second, expected);
    instance.stop_default(error::address_blocked);
}

BC_AUTO_TEST_CASE(subscriber__notify__twice__expected)
{
    test_subscriber instance{};
    constexpr auto expected = 42u;
    const auto ec = error::address_not_found;
    auto count = zero;
    auto result = true;
    std::pair<code, size_t> notify_result;

    result &= !instance.subscribe([&](code value, size_t size) NOEXCEPT
    {
        // Exclude stop_default call.
        if (++count <= two) notify_result = { value, size };
    });

    instance.notify({}, {});
    instance.notify(ec, expected);
    BC_REQUIRE(result);
    BC_REQUIRE_EQUAL(notify_result.first, ec);
    BC_REQUIRE_EQUAL(notify_result.second, expected);
    instance.stop_default(error::address_blocked);
}

BC_AUTO_TEST_SUITE_END()
