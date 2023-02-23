/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(volume_racer_tests)

using volume_racer_t = volume_racer<error::success, error::invalid_magic>;

BOOST_AUTO_TEST_CASE(volume_racer__running__empty__false)
{
    volume_racer_t volume_racer{ 0, 0 };
    BOOST_REQUIRE(!volume_racer.running());
}

BOOST_AUTO_TEST_CASE(volume_racer__running__unstarted__false)
{
    volume_racer_t volume_racer{ 2, 10 };
    BOOST_REQUIRE(!volume_racer.running());
}

BOOST_AUTO_TEST_CASE(volume_racer__start__unstarted__true_running)
{
    volume_racer_t volume_racer{ 2, 10 };
    BOOST_REQUIRE(volume_racer.start([](code) NOEXCEPT {}, [](code) NOEXCEPT{}));
    BOOST_REQUIRE(volume_racer.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(volume_racer.finish(2));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE(volume_racer.finish(4));
    BOOST_REQUIRE(!volume_racer.running());
}

BOOST_AUTO_TEST_CASE(volume_racer__start__started__false_running)
{
    volume_racer_t volume_racer{ 1, 10 };
    BOOST_REQUIRE(volume_racer.start([](code) NOEXCEPT{}, [](code) NOEXCEPT{}));
    BOOST_REQUIRE(!volume_racer.start([](code) NOEXCEPT{}, [](code) NOEXCEPT{}));
    BOOST_REQUIRE(volume_racer.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(volume_racer.finish(1));
    BOOST_REQUIRE(!volume_racer.running());
}

BOOST_AUTO_TEST_CASE(volume_racer__running__3_of_3__failed_sufficient_complete)
{
    volume_racer_t volume_racer{ 3, 10 };
    BOOST_REQUIRE(!volume_racer.running());

    code complete{ error::unknown };
    code sufficient{ error::unknown };
    BOOST_REQUIRE(volume_racer.start(
        [&](code ec) NOEXCEPT
        {
            sufficient = ec;
        },
        [&](code ec) NOEXCEPT
        {
            complete = ec;
        }));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient, error::unknown);
    BOOST_REQUIRE_EQUAL(complete, error::unknown);

    BOOST_REQUIRE(volume_racer.finish(1));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient, error::unknown);
    BOOST_REQUIRE_EQUAL(complete, error::unknown);

    BOOST_REQUIRE(volume_racer.finish(1));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient, error::unknown);
    BOOST_REQUIRE_EQUAL(complete, error::unknown);

    BOOST_REQUIRE(volume_racer.finish(1));
    BOOST_REQUIRE(!volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient, error::invalid_magic);
    BOOST_REQUIRE_EQUAL(complete, error::success);
}

BOOST_AUTO_TEST_CASE(volume_racer__running__4_of_3__false_finish)
{
    volume_racer_t volume_racer{ 3, 10 };
    BOOST_REQUIRE(!volume_racer.running());

    code complete{ error::unknown };
    code sufficient{ error::unknown };
    BOOST_REQUIRE(volume_racer.start(
        [&](code ec) NOEXCEPT
        {
            sufficient = ec;
        },
        [&](code ec) NOEXCEPT
        {
            complete = ec;
        }));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE(volume_racer.finish(1));
    BOOST_REQUIRE(volume_racer.finish(1));
    BOOST_REQUIRE(volume_racer.finish(1));
    BOOST_REQUIRE(!volume_racer.finish(1));
}

BOOST_AUTO_TEST_CASE(volume_racer__finish__early_sufficiency__resources_deleted_as_expected)
{
    struct destructor
    {
        using ptr = std::shared_ptr<destructor>;
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    volume_racer_t volume_racer{ 3, 10 };
    BOOST_REQUIRE(!volume_racer.running());

    bool foo_deleted{ false };
    bool bar_deleted{ false };
    auto foo = std::make_shared<destructor>(foo_deleted);
    auto bar = std::make_shared<destructor>(bar_deleted);
    std::pair<code, bool> sufficient{ error::unknown, false };
    std::pair<code, bool> complete{ error::unknown, false };

    // foo/bar captured into handlers.
    BOOST_REQUIRE(volume_racer.start(
        [=, &sufficient](code ec) NOEXCEPT
        {
            sufficient = { ec, !foo->deleted_ };
        },
        [=, &complete](code ec) NOEXCEPT
        {
            complete = { ec, !bar->deleted_ };
        }));

    // volume_racer not sufficient/complete, resources retained.
    foo.reset();
    bar.reset();
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // First finish is neither sufficient nor complete.
    BOOST_REQUIRE(volume_racer.finish(5));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Second finish is sufficient but not complete.
    BOOST_REQUIRE(volume_racer.finish(10));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::success);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Third finish is complete.
    BOOST_REQUIRE(volume_racer.finish(42));
    BOOST_REQUIRE(!volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::success);
    BOOST_REQUIRE_EQUAL(complete.first, error::success);
    BOOST_REQUIRE(sufficient.second);
    BOOST_REQUIRE(complete.second);
    BOOST_REQUIRE(foo_deleted);
    BOOST_REQUIRE(bar_deleted);
}

BOOST_AUTO_TEST_CASE(volume_racer__finish__late_insufficiency__resources_deleted_as_expected)
{
    struct destructor
    {
        using ptr = std::shared_ptr<destructor>;
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    volume_racer_t volume_racer{ 3, 10 };
    BOOST_REQUIRE(!volume_racer.running());

    bool foo_deleted{ false };
    bool bar_deleted{ false };
    auto foo = std::make_shared<destructor>(foo_deleted);
    auto bar = std::make_shared<destructor>(bar_deleted);
    std::pair<code, bool> sufficient{ error::unknown, false };
    std::pair<code, bool> complete{ error::unknown, false };

    // foo/bar captured into handlers.
    BOOST_REQUIRE(volume_racer.start(
        [=, &sufficient](code ec) NOEXCEPT
        {
            sufficient = { ec, !foo->deleted_ };
        },
        [=, &complete](code ec) NOEXCEPT
        {
            complete = { ec, !bar->deleted_ };
        }));

    // volume_racer not sufficient/complete, resources retained.
    foo.reset();
    bar.reset();
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // First finish is neither sufficient nor complete.
    BOOST_REQUIRE(volume_racer.finish(5));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Second finish is neither sufficient nor complete.
    BOOST_REQUIRE(volume_racer.finish(9));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Third finish is insufficient and complete.
    BOOST_REQUIRE(volume_racer.finish(9));
    BOOST_REQUIRE(!volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::invalid_magic);
    BOOST_REQUIRE_EQUAL(complete.first, error::success);
    BOOST_REQUIRE(sufficient.second);
    BOOST_REQUIRE(complete.second);
    BOOST_REQUIRE(foo_deleted);
    BOOST_REQUIRE(bar_deleted);
}

BOOST_AUTO_TEST_CASE(volume_racer__finish__late_sufficiency__resources_deleted_as_expected)
{
    struct destructor
    {
        using ptr = std::shared_ptr<destructor>;
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    volume_racer_t volume_racer{ 3, 10 };
    BOOST_REQUIRE(!volume_racer.running());

    bool foo_deleted{ false };
    bool bar_deleted{ false };
    auto foo = std::make_shared<destructor>(foo_deleted);
    auto bar = std::make_shared<destructor>(bar_deleted);
    std::pair<code, bool> sufficient{ error::unknown, false };
    std::pair<code, bool> complete{ error::unknown, false };

    // foo/bar captured into handlers.
    BOOST_REQUIRE(volume_racer.start(
        [=, &sufficient](code ec) NOEXCEPT
        {
            sufficient = { ec, !foo->deleted_ };
        },
        [=, &complete](code ec) NOEXCEPT
        {
            complete = { ec, !bar->deleted_ };
        }));

    // volume_racer not sufficient/complete, resources retained.
    foo.reset();
    bar.reset();
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // First finish is neither sufficient nor complete.
    BOOST_REQUIRE(volume_racer.finish(5));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Second finish is neither sufficient nor complete.
    BOOST_REQUIRE(volume_racer.finish(9));
    BOOST_REQUIRE(volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::unknown);
    BOOST_REQUIRE_EQUAL(complete.first, error::unknown);
    BOOST_REQUIRE(!sufficient.second);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!foo_deleted);
    BOOST_REQUIRE(!bar_deleted);

    // Third finish is sufficient and complete.
    BOOST_REQUIRE(volume_racer.finish(10));
    BOOST_REQUIRE(!volume_racer.running());
    BOOST_REQUIRE_EQUAL(sufficient.first, error::success);
    BOOST_REQUIRE_EQUAL(complete.first, error::success);
    BOOST_REQUIRE(sufficient.second);
    BOOST_REQUIRE(complete.second);
    BOOST_REQUIRE(foo_deleted);
    BOOST_REQUIRE(bar_deleted);
}

BOOST_AUTO_TEST_SUITE_END()
