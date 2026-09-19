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
#include "../../test.hpp"

BOOST_AUTO_TEST_SUITE(race_any_tests)

using race_any_t = race_any<const code&>;

BOOST_AUTO_TEST_CASE(race_any__destruct__unfinished__operation_failed)
{
    code complete{ error::invalid_magic };

    {
        race_any_t race_any{ [&](const code& ec) NOEXCEPT { complete = ec; } };
        BOOST_REQUIRE_EQUAL(complete, error::invalid_magic);
    }

    BOOST_REQUIRE_EQUAL(complete, error::operation_failed);
}

BOOST_AUTO_TEST_CASE(race_any__finish__first__true_expected_invocation)
{
    code complete{ error::invalid_magic };
    race_any_t race_any{ [&](const code& ec) NOEXCEPT { complete = ec; } };

    BOOST_REQUIRE(race_any.finish(error::accept_failed));
    BOOST_REQUIRE_EQUAL(complete, error::accept_failed);
}

BOOST_AUTO_TEST_CASE(race_any__finish__subsequent__false_not_invoked)
{
    code complete{ error::invalid_magic };
    race_any_t race_any{ [&](const code& ec) NOEXCEPT { complete = ec; } };

    BOOST_REQUIRE(race_any.finish(error::accept_failed));
    BOOST_REQUIRE(!race_any.finish(error::address_invalid));
    BOOST_REQUIRE(!race_any.finish(error::success));
    BOOST_REQUIRE_EQUAL(complete, error::accept_failed);
}

BOOST_AUTO_TEST_CASE(race_any__destruct__finished__not_reinvoked)
{
    code complete{ error::invalid_magic };

    {
        race_any_t race_any{ [&](const code& ec) NOEXCEPT { complete = ec; } };
        BOOST_REQUIRE(race_any.finish(error::success));
    }

    BOOST_REQUIRE_EQUAL(complete, error::success);
}

BOOST_AUTO_TEST_CASE(race_any__finish__winner__resources_deleted)
{
    struct destructor
    {
        using ptr = std::shared_ptr<destructor>;
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    bool deleted{ false };
    auto foo = std::make_shared<destructor>(deleted);
    bool complete{ false };

    // foo captured into handler.
    race_any_t race_any{ [=, &complete](const code&) NOEXCEPT { complete = !foo->deleted_; } };

    foo.reset();
    BOOST_REQUIRE(!deleted);
    BOOST_REQUIRE(race_any.finish(error::success));
    BOOST_REQUIRE(complete);
    BOOST_REQUIRE(deleted);
}

BOOST_AUTO_TEST_SUITE_END()
