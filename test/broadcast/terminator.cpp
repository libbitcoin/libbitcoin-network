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

BOOST_AUTO_TEST_SUITE(terminator_tests)

static terminator::race::ptr unused_race() NOEXCEPT
{
    return std::make_shared<terminator::race>([](const code&) NOEXCEPT {});
}

BOOST_AUTO_TEST_CASE(terminator__targets__identifier__expected)
{
    const config::address address{ "42.42.42.42:4242" };
    const terminator instance{ unused_race(), error::channel_stopped, 42 };

    BOOST_REQUIRE(instance.targets(42, address));
    BOOST_REQUIRE(!instance.targets(43, address));
}

BOOST_AUTO_TEST_CASE(terminator__targets__address__expected)
{
    const config::address address{ "42.42.42.42:4242" };
    const config::address other{ "24.24.24.24:4242" };
    const terminator instance{ unused_race(), error::channel_stopped, 0, address };

    BOOST_REQUIRE(instance.targets(42, address));
    BOOST_REQUIRE(!instance.targets(42, other));
}

BOOST_AUTO_TEST_CASE(terminator__targets__unspecified_port__any_port)
{
    const config::address address{ "42.42.42.42" };
    const config::address other{ "24.24.24.24:4242" };
    const terminator instance{ unused_race(), error::channel_stopped, 0, address };

    BOOST_REQUIRE(instance.targets(42, config::address{ "42.42.42.42:4242" }));
    BOOST_REQUIRE(instance.targets(42, config::address{ "42.42.42.42:2424" }));
    BOOST_REQUIRE(!instance.targets(42, other));
}

BOOST_AUTO_TEST_CASE(terminator__reason__always__expected)
{
    const terminator instance{ unused_race(), error::channel_dropped, 42 };
    BOOST_REQUIRE_EQUAL(instance.reason(), error::channel_dropped);
}

BOOST_AUTO_TEST_CASE(terminator__stopped__always__race_success)
{
    code complete{ error::invalid_magic };
    auto race = std::make_shared<terminator::race>([&](const code& ec) NOEXCEPT { complete = ec; });

    {
        const terminator instance{ race, error::channel_stopped, 42 };
        race.reset();
        instance.stopped();
        BOOST_REQUIRE_EQUAL(complete, error::success);
    }
}

BOOST_AUTO_TEST_CASE(terminator__destruct__unstopped__race_operation_failed)
{
    code complete{ error::invalid_magic };
    auto race = std::make_shared<terminator::race>([&](const code& ec) NOEXCEPT { complete = ec; });

    {
        const terminator instance{ race, error::channel_stopped, 42 };
        race.reset();
        BOOST_REQUIRE_EQUAL(complete, error::invalid_magic);
    }

    BOOST_REQUIRE_EQUAL(complete, error::operation_failed);
}

BOOST_AUTO_TEST_SUITE_END()
