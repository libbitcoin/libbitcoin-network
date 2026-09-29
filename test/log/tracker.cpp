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

BOOST_AUTO_TEST_SUITE(tracker_tests)

// Started log with tracker is unsafe unless blocked on write completion.
// As the object is destroyed a job is created on an independent thread.

class tracked
  : tracker<tracked>
{
public:
    tracked(const logger& log) NOEXCEPT
      : tracker<tracked>(log)
    {
    }

    bool method() const NOEXCEPT
    {
        return true;
    };
};

#if defined(HAVE_LOGO) && !defined(NDEBUG)
BOOST_AUTO_TEST_CASE(tracker__construct__guarded__safe_expected_messages)
{
    logger log{};
    std::promise<code> log_stopped{};
    auto count = zero;
    auto result = true;

    log.subscribe_messages(
        [&](const code& ec, uint8_t, time_t, const std::string& message) NOEXCEPT
        {
            if (is_zero(count++))
            {
                const auto expected = std::string{ typeid(tracked).name() } + "(1)\n";
                result &= (message == expected);
                return true;
            }
            else
            {
                const auto expected = std::string{ typeid(tracked).name() } + "(0)~\n";
                result &= (message == expected);
                log_stopped.set_value(ec);
                return false;
            }
        });

    auto instance = system::to_shared<tracked>(log);
    BOOST_REQUIRE(instance->method());

    instance.reset();
    BOOST_REQUIRE_EQUAL(log_stopped.get_future().get(), error::success);

    log.stop();
    BOOST_REQUIRE(result);
}
#endif

BOOST_AUTO_TEST_CASE(tracker__construct__stopped_log__safe)
{
    logger log{};
    log.stop();
    tracked instance{ log };
    BOOST_REQUIRE(instance.method());
}

BOOST_AUTO_TEST_SUITE_END()

BOOST_AUTO_TEST_SUITE(logger_tests)

BOOST_AUTO_TEST_CASE(logger__subscribe_messages__stopped__service_stopped)
{
    logger log{};
    log.stop();

    code result{};
    log.subscribe_messages([&](const code& ec, uint8_t, time_t, const std::string&) NOEXCEPT
    {
        result = ec;
        return false;
    });

    BOOST_REQUIRE_EQUAL(result, error::service_stopped);
}

BOOST_AUTO_TEST_CASE(logger__subscribe_events__stopped__service_stopped)
{
    logger log{};
    log.stop();

    code result{};
    log.subscribe_events([&](const code& ec, uint8_t, uint64_t, const auto&) NOEXCEPT
    {
        result = ec;
        return false;
    });

    BOOST_REQUIRE_EQUAL(result, error::service_stopped);
}

BOOST_AUTO_TEST_CASE(logger__fire__subscribed__notified_then_stopped)
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

    BOOST_REQUIRE_EQUAL(events.size(), 2u);
    BOOST_REQUIRE_EQUAL(std::get<0>(events.at(0)), error::success);
    BOOST_REQUIRE_EQUAL(std::get<1>(events.at(0)), 42u);
    BOOST_REQUIRE_EQUAL(std::get<2>(events.at(0)), 7u);
    BOOST_REQUIRE_EQUAL(std::get<0>(events.at(1)), error::service_stopped);
}

BOOST_AUTO_TEST_SUITE_END()
