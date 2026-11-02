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

BC_AUTO_TEST_SUITE(race_volume_tests)

using race_volume_t = race_volume<error::success, error::invalid_magic>;

BC_AUTO_TEST_CASE(race_volume__running__empty__false)
{
    race_volume_t race_volume{ 0, 0 };
    BC_REQUIRE(!race_volume.running());
}

BC_AUTO_TEST_CASE(race_volume__running__unstarted__false)
{
    race_volume_t race_volume{ 2, 10 };
    BC_REQUIRE(!race_volume.running());
}

BC_AUTO_TEST_CASE(race_volume__start__unstarted__true_running)
{
    race_volume_t race_volume{ 2, 10 };
    BC_REQUIRE(race_volume.start([](code) NOEXCEPT {}, [](code) NOEXCEPT{}));
    BC_REQUIRE(race_volume.running());

    // Avoid running at destruct assertion.
    BC_REQUIRE(!race_volume.finish(2));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE(!race_volume.finish(4));
    BC_REQUIRE(!race_volume.running());
}

BC_AUTO_TEST_CASE(race_volume__start__started__false_running)
{
    race_volume_t race_volume{ 1, 10 };
    BC_REQUIRE(race_volume.start([](code) NOEXCEPT{}, [](code) NOEXCEPT{}));
    BC_REQUIRE(!race_volume.start([](code) NOEXCEPT{}, [](code) NOEXCEPT{}));
    BC_REQUIRE(race_volume.running());

    // Avoid running at destruct assertion.
    BC_REQUIRE(!race_volume.finish(1));
    BC_REQUIRE(!race_volume.running());
}

BC_AUTO_TEST_CASE(race_volume__running__3_of_3__insufficient_complete)
{
    race_volume_t race_volume{ 3, 10 };
    BC_REQUIRE(!race_volume.running());

    code complete{ error::unknown };
    code sufficient{ error::unknown };
    BC_REQUIRE(race_volume.start(
        [&](code ec) NOEXCEPT
        {
            sufficient = ec;
        },
        [&](code ec) NOEXCEPT
        {
            complete = ec;
        }));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE_EQUAL(sufficient, error::unknown);
    BC_REQUIRE_EQUAL(complete, error::unknown);

    BC_REQUIRE(!race_volume.finish(1));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE_EQUAL(sufficient, error::unknown);
    BC_REQUIRE_EQUAL(complete, error::unknown);

    BC_REQUIRE(!race_volume.finish(1));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE_EQUAL(sufficient, error::unknown);
    BC_REQUIRE_EQUAL(complete, error::unknown);

    BC_REQUIRE(!race_volume.finish(1));
    BC_REQUIRE(!race_volume.running());
    BC_REQUIRE_EQUAL(sufficient, error::invalid_magic);
    BC_REQUIRE_EQUAL(complete, error::success);
}

BC_AUTO_TEST_CASE(race_volume__running__4_of_3__insufficient)
{
    race_volume_t race_volume{ 3, 10 };
    BC_REQUIRE(!race_volume.running());

    code complete{ error::unknown };
    code sufficient{ error::unknown };
    BC_REQUIRE(race_volume.start(
        [&](code ec) NOEXCEPT
        {
            sufficient = ec;
        },
        [&](code ec) NOEXCEPT
        {
            complete = ec;
        }));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE(!race_volume.finish(1));
    BC_REQUIRE(!race_volume.finish(1));
    BC_REQUIRE(!race_volume.finish(1));
    BC_REQUIRE(!race_volume.finish(1));
}

BC_AUTO_TEST_CASE(race_volume__finish__early_sufficiency__resources_deleted_as_expected)
{
    struct destructor
    {
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    race_volume_t race_volume{ 3, 10 };
    BC_REQUIRE(!race_volume.running());

    bool foo_deleted{ false };
    bool bar_deleted{ false };
    auto foo = std::make_shared<destructor>(foo_deleted);
    auto bar = std::make_shared<destructor>(bar_deleted);
    std::pair<code, bool> sufficient{ error::unknown, false };
    std::pair<code, bool> complete{ error::unknown, false };

    // foo/bar captured into handlers.
    BC_REQUIRE(race_volume.start(
        [=, &sufficient](code ec) NOEXCEPT
        {
            sufficient = { ec, !foo->deleted_ };
        },
        [=, &complete](code ec) NOEXCEPT
        {
            complete = { ec, !bar->deleted_ };
        }));

    // race_volume not sufficient/complete, resources retained.
    foo.reset();
    bar.reset();
    BC_REQUIRE(!foo_deleted);
    BC_REQUIRE(!bar_deleted);

    // First finish is neither sufficient nor complete.
    BC_REQUIRE(!race_volume.finish(5));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BC_REQUIRE_EQUAL(complete.first, error::unknown);
    BC_REQUIRE(!sufficient.second);
    BC_REQUIRE(!complete.second);
    BC_REQUIRE(!foo_deleted);
    BC_REQUIRE(!bar_deleted);

    // Second finish is sufficient but not complete.
    BC_REQUIRE(race_volume.finish(10));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE_EQUAL(sufficient.first, error::success);
    BC_REQUIRE_EQUAL(complete.first, error::unknown);
    BC_REQUIRE(sufficient.second);
    BC_REQUIRE(!complete.second);
    BC_REQUIRE(foo_deleted);
    BC_REQUIRE(!bar_deleted);

    // Third finish is complete.
    BC_REQUIRE(!race_volume.finish(42));
    BC_REQUIRE(!race_volume.running());
    BC_REQUIRE_EQUAL(sufficient.first, error::success);
    BC_REQUIRE_EQUAL(complete.first, error::success);
    BC_REQUIRE(sufficient.second);
    BC_REQUIRE(complete.second);
    BC_REQUIRE(foo_deleted);
    BC_REQUIRE(bar_deleted);
}

BC_AUTO_TEST_CASE(race_volume__finish__late_insufficiency__resources_deleted_as_expected)
{
    struct destructor
    {
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    race_volume_t race_volume{ 3, 10 };
    BC_REQUIRE(!race_volume.running());

    bool foo_deleted{ false };
    bool bar_deleted{ false };
    auto foo = std::make_shared<destructor>(foo_deleted);
    auto bar = std::make_shared<destructor>(bar_deleted);
    std::pair<code, bool> sufficient{ error::unknown, false };
    std::pair<code, bool> complete{ error::unknown, false };

    // foo/bar captured into handlers.
    BC_REQUIRE(race_volume.start(
        [=, &sufficient](code ec) NOEXCEPT
        {
            sufficient = { ec, !foo->deleted_ };
        },
        [=, &complete](code ec) NOEXCEPT
        {
            complete = { ec, !bar->deleted_ };
        }));

    // race_volume not sufficient/complete, resources retained.
    foo.reset();
    bar.reset();
    BC_REQUIRE(!foo_deleted);
    BC_REQUIRE(!bar_deleted);

    // First finish is neither sufficient nor complete.
    BC_REQUIRE(!race_volume.finish(5));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BC_REQUIRE_EQUAL(complete.first, error::unknown);
    BC_REQUIRE(!sufficient.second);
    BC_REQUIRE(!complete.second);
    BC_REQUIRE(!foo_deleted);
    BC_REQUIRE(!bar_deleted);

    // Second finish is neither sufficient nor complete.
    BC_REQUIRE(!race_volume.finish(9));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BC_REQUIRE_EQUAL(complete.first, error::unknown);
    BC_REQUIRE(!sufficient.second);
    BC_REQUIRE(!complete.second);
    BC_REQUIRE(!foo_deleted);
    BC_REQUIRE(!bar_deleted);

    // Third finish is insufficient and complete.
    BC_REQUIRE(!race_volume.finish(9));
    BC_REQUIRE(!race_volume.running());
    BC_REQUIRE_EQUAL(sufficient.first, error::invalid_magic);
    BC_REQUIRE_EQUAL(complete.first, error::success);
    BC_REQUIRE(sufficient.second);
    BC_REQUIRE(complete.second);
    BC_REQUIRE(foo_deleted);
    BC_REQUIRE(bar_deleted);
}

BC_AUTO_TEST_CASE(race_volume__finish__late_sufficiency__resources_deleted_as_expected)
{
    struct destructor
    {
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    race_volume_t race_volume{ 3, 10 };
    BC_REQUIRE(!race_volume.running());

    bool foo_deleted{ false };
    bool bar_deleted{ false };
    auto foo = std::make_shared<destructor>(foo_deleted);
    auto bar = std::make_shared<destructor>(bar_deleted);
    std::pair<code, bool> sufficient{ error::unknown, false };
    std::pair<code, bool> complete{ error::unknown, false };

    // foo/bar captured into handlers.
    BC_REQUIRE(race_volume.start(
        [=, &sufficient](code ec) NOEXCEPT
        {
            sufficient = { ec, !foo->deleted_ };
        },
        [=, &complete](code ec) NOEXCEPT
        {
            complete = { ec, !bar->deleted_ };
        }));

    // race_volume not sufficient/complete, resources retained.
    foo.reset();
    bar.reset();
    BC_REQUIRE(!foo_deleted);
    BC_REQUIRE(!bar_deleted);

    // First finish is neither sufficient nor complete.
    BC_REQUIRE(!race_volume.finish(5));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BC_REQUIRE_EQUAL(complete.first, error::unknown);
    BC_REQUIRE(!sufficient.second);
    BC_REQUIRE(!complete.second);
    BC_REQUIRE(!foo_deleted);
    BC_REQUIRE(!bar_deleted);

    // Second finish is neither sufficient nor complete.
    BC_REQUIRE(!race_volume.finish(9));
    BC_REQUIRE(race_volume.running());
    BC_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BC_REQUIRE_EQUAL(complete.first, error::unknown);
    BC_REQUIRE(!sufficient.second);
    BC_REQUIRE(!complete.second);
    BC_REQUIRE(!foo_deleted);
    BC_REQUIRE(!bar_deleted);

    // Third finish is sufficient and complete.
    BC_REQUIRE(race_volume.finish(10));
    BC_REQUIRE(!race_volume.running());
    BC_REQUIRE_EQUAL(sufficient.first, error::success);
    BC_REQUIRE_EQUAL(complete.first, error::success);
    BC_REQUIRE(sufficient.second);
    BC_REQUIRE(complete.second);
    BC_REQUIRE(foo_deleted);
    BC_REQUIRE(bar_deleted);
}

BC_AUTO_TEST_SUITE_END()
