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

BC_AUTO_TEST_SUITE(race_all_tests)

using race_all_t = race_all<const code&>;

BC_AUTO_TEST_CASE(race_all__destruct__scoped__success)
{
    code complete{ error::invalid_magic };

    {
        race_all_t race_all{ [&](const code& ec) NOEXCEPT { complete = ec; } };
        BC_REQUIRE_EQUAL(complete, error::invalid_magic);
    }

    BC_REQUIRE_EQUAL(complete, error::success);
}

BC_AUTO_TEST_CASE(race_all__destruct__referenced__incomplete)
{
    code complete{ error::invalid_magic };
    auto race = std::make_shared<race_all_t>([&](const code& ec) NOEXCEPT { complete = ec; });
    auto copy = race;

    race.reset();
    BC_REQUIRE_EQUAL(complete, error::invalid_magic);

    copy.reset();
    BC_REQUIRE_EQUAL(complete, error::success);
}

BC_AUTO_TEST_CASE(race_all__destruct__captured__resources_deleted)
{
    struct destructor
    {
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
        BC_REQUIRE(!deleted);
    }

    BC_REQUIRE(complete);
    BC_REQUIRE(deleted);
}

BC_AUTO_TEST_SUITE_END()
