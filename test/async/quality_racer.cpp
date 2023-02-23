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

BOOST_AUTO_TEST_SUITE(quality_racer_tests)

using quality_racer_t = quality_racer<const code&, size_t>;

BOOST_AUTO_TEST_CASE(quality_racer__running__empty__false)
{
    quality_racer_t quality_racer{ 0 };
    BOOST_REQUIRE(!quality_racer.running());
}

BOOST_AUTO_TEST_CASE(quality_racer__running__unstarted__false)
{
    quality_racer_t quality_racer{ 2 };
    BOOST_REQUIRE(!quality_racer.running());
}

BOOST_AUTO_TEST_CASE(quality_racer__start__unstarted__true_running)
{
    quality_racer_t quality_racer{ 3 };
    BOOST_REQUIRE(quality_racer.start([&](code, size_t) NOEXCEPT {}));
    BOOST_REQUIRE(quality_racer.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(quality_racer.finish({}, {}));
    BOOST_REQUIRE(!quality_racer.finish({}, {}));
    BOOST_REQUIRE(!quality_racer.finish({}, {}));
    BOOST_REQUIRE(!quality_racer.running());
}

BOOST_AUTO_TEST_CASE(quality_racer__start__started__false_running)
{
    quality_racer_t quality_racer{ 3 };
    BOOST_REQUIRE(quality_racer.start([&](code, size_t) NOEXCEPT {}));
    BOOST_REQUIRE(!quality_racer.start([&](code, size_t) NOEXCEPT {}));
    BOOST_REQUIRE(quality_racer.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(quality_racer.finish({}, {}));
    BOOST_REQUIRE(!quality_racer.finish({}, {}));
    BOOST_REQUIRE(!quality_racer.finish({}, {}));
    BOOST_REQUIRE(!quality_racer.running());
}

BOOST_AUTO_TEST_CASE(quality_racer__running__3_of_3__false_expected_invocation)
{
    const std::pair<code, size_t> expected{ error::invalid_magic, 3 };
    std::pair<code, size_t> complete{};
    quality_racer_t quality_racer{ 3 };

    BOOST_REQUIRE(!quality_racer.running());
    BOOST_REQUIRE(quality_racer.start([&](code ec, size_t size) NOEXCEPT
    {
        complete = { ec, size };
    }));

    BOOST_REQUIRE(quality_racer.running());
    BOOST_REQUIRE(!quality_racer.finish(error::address_invalid, 1));
    BOOST_REQUIRE(quality_racer.running());
    BOOST_REQUIRE(!quality_racer.finish(error::accept_failed, 2));
    BOOST_REQUIRE(quality_racer.running());
    BOOST_REQUIRE(!quality_racer.finish(expected.first, expected.second));
    BOOST_REQUIRE(!quality_racer.running());
    BOOST_REQUIRE_EQUAL(complete.first, expected.first);
    BOOST_REQUIRE_EQUAL(complete.second, expected.second);
}

BOOST_AUTO_TEST_CASE(quality_racer__running__4_of_3__false_expected_invocation)
{
    const std::pair<code, size_t> expected{ error::invalid_magic, 3 };
    std::pair<code, size_t> complete{};
    quality_racer_t quality_racer{ 3 };

    BOOST_REQUIRE(!quality_racer.running());
    BOOST_REQUIRE(quality_racer.start([&](code ec, size_t size) NOEXCEPT
    {
        complete = { ec, size };
    }));

    BOOST_REQUIRE(quality_racer.running());
    BOOST_REQUIRE(!quality_racer.finish(error::accept_failed, 1));
    BOOST_REQUIRE(!quality_racer.finish(error::address_invalid, 2));
    BOOST_REQUIRE(!quality_racer.finish(expected.first, expected.second));
    BOOST_REQUIRE(!quality_racer.running());
    BOOST_REQUIRE(!quality_racer.finish(error::success, 4));
    BOOST_REQUIRE(!quality_racer.running());
    BOOST_REQUIRE_EQUAL(complete.first, expected.first);
    BOOST_REQUIRE_EQUAL(complete.second, expected.second);
}

BOOST_AUTO_TEST_CASE(quality_racer__finish__3_of_3__resources_deleted)
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
    quality_racer<const code&, const destructor::ptr&> quality_racer{ 3 };
    std::pair<bool, bool> complete{};

    // foo/bar captured/passed into handler.
    BOOST_REQUIRE(quality_racer.start([=, &complete](const code&, const destructor::ptr& bar) NOEXCEPT
    {
        complete = { !foo->deleted_, !bar->deleted_ };
    }));

    // First finish is winner, captures foo.
    BOOST_REQUIRE(quality_racer.finish(error::success, foo));
    BOOST_REQUIRE(quality_racer.running());
    BOOST_REQUIRE(!complete.first);
    BOOST_REQUIRE(!complete.second);

    // quality_racer not finished, resources retained.
    foo.reset();
    BOOST_REQUIRE(!deleted);

    // quality_racer not finished, resources retained.
    BOOST_REQUIRE(!quality_racer.finish(error::success, {}));
    BOOST_REQUIRE(quality_racer.running());
    BOOST_REQUIRE(!complete.first);
    BOOST_REQUIRE(!complete.second);
    BOOST_REQUIRE(!deleted);

    // quality_racer finished (invoked with no winner), resourced cleared.
    BOOST_REQUIRE(!quality_racer.finish(error::success, {}));
    BOOST_REQUIRE(!quality_racer.running());
    BOOST_REQUIRE(complete.first);
    BOOST_REQUIRE(complete.second);
    BOOST_REQUIRE(deleted);
}

BOOST_AUTO_TEST_SUITE_END()
