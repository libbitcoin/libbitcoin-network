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

BC_AUTO_TEST_SUITE(tracker_tests)

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
BC_AUTO_TEST_CASE(tracker__construct__guarded__construct_message)
{
    using namespace system;

    const logger log{};
    std::promise<std::string> notified{};
    const auto value = bit_right<uint64_t>(levels::objects);
    const auto interest = emplace_shared<logger::interest>(log, value);

    log.subscribe_messages(interest, [&](const code&, uint8_t, time_t, const auto& message) NOEXCEPT
    {
        notified.set_value(message);
        return false;
    });

    const auto instance = to_shared<tracked>(log);
    const auto expected = std::string{ typeid(tracked).name() } + "(1)\n";
    BC_REQUIRE_EQUAL(notified.get_future().get(), expected);
}

BC_AUTO_TEST_CASE(tracker__destruct__guarded__destruct_message)
{
    using namespace system;

    const logger log{};
    std::promise<std::string> notified{};
    const auto value = bit_right<uint64_t>(levels::objects);
    const auto interest = emplace_shared<logger::interest>(log, value);
    auto instance = to_shared<tracked>(log);

    log.subscribe_messages(interest, [&](const code&, uint8_t, time_t, const auto& message) NOEXCEPT
    {
        notified.set_value(message);
        return false;
    });

    instance.reset();
    const auto expected = std::string{ typeid(tracked).name() } + "(0)~\n";
    BC_REQUIRE_EQUAL(notified.get_future().get(), expected);
}
#endif

BC_AUTO_TEST_CASE(tracker__construct__stopped_log__safe)
{
    logger log{};
    log.stop();
    tracked instance{ log };
    BC_REQUIRE(instance.method());
}

BC_AUTO_TEST_SUITE_END()
