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

BC_AUTO_TEST_SUITE(logger_tests)

BC_AUTO_TEST_CASE(logger__enabled__default__false)
{
    const logger log{};
    BC_REQUIRE(!log.enabled(levels::application));
    BC_REQUIRE(!log.enabled(levels::proxy));
    BC_REQUIRE(!log.enabled(levels::verbose));
}

BC_AUTO_TEST_CASE(logger__enabled__unmaskable_level__false)
{
    const logger log{};
    const logger::interest interest{ log, max_uint64 };
    BC_REQUIRE(log.enabled(sub1(bits<uint64_t>)));
    BC_REQUIRE(!log.enabled(bits<uint64_t>));
}

BC_AUTO_TEST_CASE(logger__interest__construct__only_masked_levels_enabled)
{
    const logger log{};
    const logger::interest interest{ log, system::bit_right<uint64_t>(levels::proxy) };
    BC_REQUIRE(log.enabled(levels::proxy));
    BC_REQUIRE(!log.enabled(levels::remote));
}

BC_AUTO_TEST_CASE(logger__interest__destruct__level_disabled)
{
    const logger log{};
    auto interest = system::emplace_shared<logger::interest>(log, system::bit_right<uint64_t>(levels::proxy));
    interest.reset();
    BC_REQUIRE(!log.enabled(levels::proxy));
}

BC_AUTO_TEST_CASE(logger__interest__enable__level_enabled)
{
    const logger log{};
    logger::interest interest{ log };
    interest.enable(system::bit_right<uint64_t>(levels::proxy));
    BC_REQUIRE(log.enabled(levels::proxy));
    BC_REQUIRE(!log.enabled(levels::remote));
}

BC_AUTO_TEST_CASE(logger__interest__disable_one_of_two__enabled)
{
    const logger log{};
    const auto proxy = system::bit_right<uint64_t>(levels::proxy);
    logger::interest first{ log, proxy };
    const logger::interest second{ log, proxy };
    first.disable(proxy);
    BC_REQUIRE(log.enabled(levels::proxy));
}

BC_AUTO_TEST_CASE(logger__interest__set__levels_replaced)
{
    const logger log{};
    const auto proxy = system::bit_right<uint64_t>(levels::proxy);
    const auto remote = system::bit_right<uint64_t>(levels::remote);
    logger::interest interest{ log, proxy };
    const auto previous = interest.set(remote);
    BC_REQUIRE_EQUAL(previous, proxy);
    BC_REQUIRE(!log.enabled(levels::proxy));
    BC_REQUIRE(log.enabled(levels::remote));
}

BC_AUTO_TEST_CASE(logger__interest__toggle__levels_flipped)
{
    const logger log{};
    const auto proxy = system::bit_right<uint64_t>(levels::proxy);
    const auto remote = system::bit_right<uint64_t>(levels::remote);
    logger::interest interest{ log, proxy };
    const auto previous = interest.toggle(system::bit_or(proxy, remote));
    BC_REQUIRE_EQUAL(previous, proxy);
    BC_REQUIRE(!log.enabled(levels::proxy));
    BC_REQUIRE(log.enabled(levels::remote));
}

BC_AUTO_TEST_CASE(logger__subscribe_messages__uninterested_level__not_notified)
{
    const logger log{};
    std::promise<std::string> notified{};
    const auto interest = system::emplace_shared<logger::interest>(log, system::bit_right<uint64_t>(levels::remote));
    log.subscribe_messages(interest, [&](const code&, uint8_t, time_t, const std::string& message) NOEXCEPT
    {
        notified.set_value(message);
        return false;
    });

    log.write(levels::proxy) << "dropped";
    log.write(levels::remote) << "matched";
    BC_REQUIRE_EQUAL(notified.get_future().get(), "matched");
}

BC_AUTO_TEST_CASE(logger__subscribe_messages__interest_narrowed__not_notified)
{
    const logger log{};
    const auto proxy = system::bit_right<uint64_t>(levels::proxy);
    const auto remote = system::bit_right<uint64_t>(levels::remote);
    std::promise<std::string> notified{};
    const auto interest = system::emplace_shared<logger::interest>(log, system::bit_or(proxy, remote));
    log.subscribe_messages(interest, [&](const code&, uint8_t, time_t, const std::string& message) NOEXCEPT
    {
        notified.set_value(message);
        return false;
    });

    interest->disable(proxy);
    log.write(levels::proxy) << "dropped";
    log.write(levels::remote) << "matched";
    BC_REQUIRE_EQUAL(notified.get_future().get(), "matched");
}

BC_AUTO_TEST_CASE(logger__subscribe_messages__interest_destroyed__not_notified)
{
    const logger log{};
    const auto proxy = system::bit_right<uint64_t>(levels::proxy);
    auto called = false;
    auto first = system::emplace_shared<logger::interest>(log, proxy);
    log.subscribe_messages(first, [&](const code&, uint8_t, time_t, const std::string&) NOEXCEPT
    {
        called = true;
        return true;
    });

    std::promise<bool> notified{};
    const auto second = system::emplace_shared<logger::interest>(log, proxy);
    log.subscribe_messages(second, [&](const code&, uint8_t, time_t, const std::string&) NOEXCEPT
    {
        notified.set_value(true);
        return false;
    });

    first.reset();
    log.write(levels::proxy) << "message";
    BC_REQUIRE(notified.get_future().get());
    BC_REQUIRE(!called);
}

BC_AUTO_TEST_CASE(logger__subscribe_messages__handler_false__interest_released)
{
    const logger log{};
    const auto first = system::emplace_shared<logger::interest>(log, system::bit_right<uint64_t>(levels::proxy));
    log.subscribe_messages(first, [](const code&, uint8_t, time_t, const std::string&) NOEXCEPT
    {
        return false;
    });

    std::promise<bool> notified{};
    const auto second = system::emplace_shared<logger::interest>(log, system::bit_right<uint64_t>(levels::remote));
    log.subscribe_messages(second, [&](const code&, uint8_t, time_t, const std::string&) NOEXCEPT
    {
        notified.set_value(true);
        return false;
    });

    log.write(levels::proxy) << "releases";
    log.write(levels::remote) << "synchronizes";
    BC_REQUIRE(notified.get_future().get());
    BC_REQUIRE(!log.enabled(levels::proxy));
}

BC_AUTO_TEST_CASE(logger__subscribe_messages__empty_interest__notified_of_stop)
{
    logger log{};
    std::promise<code> stopped{};
    const auto interest = system::emplace_shared<logger::interest>(log);
    log.subscribe_messages(interest, [&](const code& ec, uint8_t, time_t, const std::string&) NOEXCEPT
    {
        stopped.set_value(ec);
        return false;
    });

    log.stop();
    BC_REQUIRE_EQUAL(stopped.get_future().get(), error::service_stopped);
}

BC_AUTO_TEST_CASE(logger__log__uninterested_level__not_formatted)
{
    const logger log{};
    auto formatted = false;
    const auto format = [&]() NOEXCEPT
    {
        formatted = true;
        return 42;
    };

    LOG(proxy, format());
    BC_REQUIRE(!formatted);
}

BC_AUTO_TEST_CASE(logger__subscribe_messages__stopped__service_stopped_interest_released)
{
    logger log{};
    log.stop();

    code result{};
    const auto interest = system::emplace_shared<logger::interest>(log, system::bit_right<uint64_t>(levels::proxy));
    log.subscribe_messages(interest, [&](const code& ec, uint8_t, time_t, const std::string&) NOEXCEPT
    {
        result = ec;
        return false;
    });

    BC_REQUIRE_EQUAL(result, error::service_stopped);
    BC_REQUIRE(!log.enabled(levels::proxy));
}

BC_AUTO_TEST_CASE(logger__subscribe_events__stopped__service_stopped)
{
    logger log{};
    log.stop();

    code result{};
    log.subscribe_events([&](const code& ec, uint8_t, uint64_t, const auto&) NOEXCEPT
    {
        result = ec;
        return false;
    });

    BC_REQUIRE_EQUAL(result, error::service_stopped);
}

BC_AUTO_TEST_CASE(logger__fire__subscribed__notified_then_stopped)
{
    std::vector<std::tuple<code, uint8_t, uint64_t>> events{};

    {
        logger log{};
        log.subscribe_events([&](const code& ec, uint8_t event_, uint64_t value, const auto&) NOEXCEPT
        {
            events.emplace_back(ec, event_, value);
            return true;
        });

        log.fire(42, 7);
        log.stop();
    }

    BC_REQUIRE_EQUAL(events.size(), 2u);
    BC_REQUIRE_EQUAL(std::get<0>(events.at(0)), error::success);
    BC_REQUIRE_EQUAL(std::get<1>(events.at(0)), 42u);
    BC_REQUIRE_EQUAL(std::get<2>(events.at(0)), 7u);
    BC_REQUIRE_EQUAL(std::get<0>(events.at(1)), error::service_stopped);
}

BC_AUTO_TEST_SUITE_END()
