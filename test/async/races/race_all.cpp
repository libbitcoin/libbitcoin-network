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

BOOST_AUTO_TEST_SUITE(race_all_tests)

using race_all_t = race_all<const code&>;

BOOST_AUTO_TEST_CASE(race_all__destruct__scoped__success)
{
    code complete{ error::invalid_magic };

    {
        race_all_t race_all{ [&](const code& ec) NOEXCEPT { complete = ec; } };
        BOOST_REQUIRE_EQUAL(complete, error::invalid_magic);
    }

    BOOST_REQUIRE_EQUAL(complete, error::success);
}

BOOST_AUTO_TEST_CASE(race_all__destruct__referenced__incomplete)
{
    code complete{ error::invalid_magic };
    auto race = std::make_shared<race_all_t>([&](const code& ec) NOEXCEPT { complete = ec; });
    auto copy = race;

    race.reset();
    BOOST_REQUIRE_EQUAL(complete, error::invalid_magic);

    copy.reset();
    BOOST_REQUIRE_EQUAL(complete, error::success);
}

BOOST_AUTO_TEST_CASE(race_all__destruct__captured__resources_deleted)
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

    {
        // foo captured into handler.
        race_all_t race_all{ [=, &complete](const code&) NOEXCEPT { complete = !foo->deleted_; } };

        foo.reset();
        BOOST_REQUIRE(!deleted);
    }

    BOOST_REQUIRE(complete);
    BOOST_REQUIRE(deleted);
}

BOOST_AUTO_TEST_SUITE_END()
