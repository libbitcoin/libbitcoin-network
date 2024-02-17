/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
