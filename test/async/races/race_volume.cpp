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
#include "../../test.hpp"

BOOST_AUTO_TEST_SUITE(race_volume_tests)

using race_volume_t = race_volume<error::success, error::invalid_magic>;

BOOST_AUTO_TEST_CASE(race_volume__running__empty__false)
{
    race_volume_t race_volume{ 0, 0 };
    BOOST_REQUIRE(!race_volume.running());
}

BOOST_AUTO_TEST_CASE(race_volume__running__unstarted__false)
{
    race_volume_t race_volume{ 2, 10 };
    BOOST_REQUIRE(!race_volume.running());
}

BOOST_AUTO_TEST_CASE(race_volume__start__unstarted__true_running)
{
    race_volume_t race_volume{ 2, 10 };
    BOOST_REQUIRE(race_volume.start([](code) NOEXCEPT {}, [](code) NOEXCEPT{}));
    BOOST_REQUIRE(race_volume.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(!race_volume.finish(2));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE(!race_volume.finish(4));
    BOOST_REQUIRE(!race_volume.running());
}

BOOST_AUTO_TEST_CASE(race_volume__start__started__false_running)
{
    race_volume_t race_volume{ 1, 10 };
    BOOST_REQUIRE(race_volume.start([](code) NOEXCEPT{}, [](code) NOEXCEPT{}));
    BOOST_REQUIRE(!race_volume.start([](code) NOEXCEPT{}, [](code) NOEXCEPT{}));
    BOOST_REQUIRE(race_volume.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(!race_volume.finish(1));
    BOOST_REQUIRE(!race_volume.running());
}

BOOST_AUTO_TEST_CASE(race_volume__running__3_of_3__insufficient_complete)
{
    race_volume_t race_volume{ 3, 10 };
    BOOST_REQUIRE(!race_volume.running());

    code complete{ error::unknown };
    code sufficient{ error::unknown };
    BOOST_REQUIRE(race_volume.start(
        [&](code ec) NOEXCEPT
        {
            sufficient = ec;
        },
        [&](code ec) NOEXCEPT
        {
            complete = ec;
        }));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient, error::unknown);
    BOOST_REQUIRE_EQUAL(complete, error::unknown);

    BOOST_REQUIRE(!race_volume.finish(1));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient, error::unknown);
    BOOST_REQUIRE_EQUAL(complete, error::unknown);

    BOOST_REQUIRE(!race_volume.finish(1));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient, error::unknown);
    BOOST_REQUIRE_EQUAL(complete, error::unknown);

    BOOST_REQUIRE(!race_volume.finish(1));
    BOOST_REQUIRE(!race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient, error::invalid_magic);
    BOOST_REQUIRE_EQUAL(complete, error::success);
}

BOOST_AUTO_TEST_CASE(race_volume__running__4_of_3__insufficient)
{
    race_volume_t race_volume{ 3, 10 };
    BOOST_REQUIRE(!race_volume.running());

    code complete{ error::unknown };
    code sufficient{ error::unknown };
    BOOST_REQUIRE(race_volume.start(
        [&](code ec) NOEXCEPT
        {
            sufficient = ec;
        },
        [&](code ec) NOEXCEPT
        {
            complete = ec;
        }));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE(!race_volume.finish(1));
    BOOST_REQUIRE(!race_volume.finish(1));
    BOOST_REQUIRE(!race_volume.finish(1));
    BOOST_REQUIRE(!race_volume.finish(1));
}

BOOST_AUTO_TEST_CASE(race_volume__finish__early_sufficiency__resources_deleted_as_expected)
{
    struct destructor
    {
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    race_volume_t race_volume{ 3, 10 };
    BOOST_REQUIRE(!race_volume.running());

    bool foo_deleted{ false };
    bool bar_deleted{ false };
    auto foo = std::make_shared<destructor>(foo_deleted);
    auto bar = std::make_shared<destructor>(bar_deleted);
    std::pair<code, bool> sufficient{ error::unknown, false };
    std::pair<code, bool> complete{ error::unknown, false };

    // foo/bar captured into handlers.
    BOOST_REQUIRE(race_volume.start(
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
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // First finish is neither sufficient nor complete.
    BOOST_REQUIRE(!race_volume.finish(5));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Second finish is sufficient but not complete.
    BOOST_REQUIRE(race_volume.finish(10));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::success);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Third finish is complete.
    BOOST_REQUIRE(!race_volume.finish(42));
    BOOST_REQUIRE(!race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::success);
    BOOST_REQUIRE_EQUAL(complete.first, error::success);
    BOOST_REQUIRE(sufficient.second);
    BOOST_REQUIRE(complete.second);
    BOOST_REQUIRE(foo_deleted);
    BOOST_REQUIRE(bar_deleted);
}

BOOST_AUTO_TEST_CASE(race_volume__finish__late_insufficiency__resources_deleted_as_expected)
{
    struct destructor
    {
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    race_volume_t race_volume{ 3, 10 };
    BOOST_REQUIRE(!race_volume.running());

    bool foo_deleted{ false };
    bool bar_deleted{ false };
    auto foo = std::make_shared<destructor>(foo_deleted);
    auto bar = std::make_shared<destructor>(bar_deleted);
    std::pair<code, bool> sufficient{ error::unknown, false };
    std::pair<code, bool> complete{ error::unknown, false };

    // foo/bar captured into handlers.
    BOOST_REQUIRE(race_volume.start(
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
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // First finish is neither sufficient nor complete.
    BOOST_REQUIRE(!race_volume.finish(5));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Second finish is neither sufficient nor complete.
    BOOST_REQUIRE(!race_volume.finish(9));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Third finish is insufficient and complete.
    BOOST_REQUIRE(!race_volume.finish(9));
    BOOST_REQUIRE(!race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::invalid_magic);
    BOOST_REQUIRE_EQUAL(complete.first, error::success);
    BOOST_REQUIRE(sufficient.second);
    BOOST_REQUIRE(complete.second);
    BOOST_REQUIRE(foo_deleted);
    BOOST_REQUIRE(bar_deleted);
}

BOOST_AUTO_TEST_CASE(race_volume__finish__late_sufficiency__resources_deleted_as_expected)
{
    struct destructor
    {
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    race_volume_t race_volume{ 3, 10 };
    BOOST_REQUIRE(!race_volume.running());

    bool foo_deleted{ false };
    bool bar_deleted{ false };
    auto foo = std::make_shared<destructor>(foo_deleted);
    auto bar = std::make_shared<destructor>(bar_deleted);
    std::pair<code, bool> sufficient{ error::unknown, false };
    std::pair<code, bool> complete{ error::unknown, false };

    // foo/bar captured into handlers.
    BOOST_REQUIRE(race_volume.start(
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
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // First finish is neither sufficient nor complete.
    BOOST_REQUIRE(!race_volume.finish(5));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Second finish is neither sufficient nor complete.
    BOOST_REQUIRE(!race_volume.finish(9));
    BOOST_REQUIRE(race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Third finish is sufficient and complete.
    BOOST_REQUIRE(race_volume.finish(10));
    BOOST_REQUIRE(!race_volume.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::success);
    BOOST_REQUIRE_EQUAL(complete.first, error::success);
    BOOST_REQUIRE(sufficient.second);
    BOOST_REQUIRE(complete.second);
    BOOST_REQUIRE(foo_deleted);
    BOOST_REQUIRE(bar_deleted);
}

BOOST_AUTO_TEST_SUITE_END()
