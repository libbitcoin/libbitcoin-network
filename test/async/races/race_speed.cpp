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

BOOST_AUTO_TEST_SUITE(race_speed_tests)

using race_speed_t = race_speed<3, const code&, size_t>;

BOOST_AUTO_TEST_CASE(race_speed__running__unstarted__false)
{
    race_speed_t race_speed{};
    BOOST_REQUIRE(!race_speed.running());
}

BOOST_AUTO_TEST_CASE(race_speed__start__unstarted__true_running)
{
    race_speed_t race_speed{};
    BOOST_REQUIRE(race_speed.start([&](code, size_t) NOEXCEPT {}));
    BOOST_REQUIRE(race_speed.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(race_speed.finish({}, {}));
    BOOST_REQUIRE(!race_speed.finish({}, {}));
    BOOST_REQUIRE(!race_speed.finish({}, {}));
    BOOST_REQUIRE(!race_speed.running());
}

BOOST_AUTO_TEST_CASE(race_speed__start__started__false_running)
{
    race_speed_t race_speed{};
    BOOST_REQUIRE(race_speed.start([&](code, size_t) NOEXCEPT {}));
    BOOST_REQUIRE(!race_speed.start([&](code, size_t) NOEXCEPT {}));
    BOOST_REQUIRE(race_speed.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(race_speed.finish({}, {}));
    BOOST_REQUIRE(!race_speed.finish({}, {}));
    BOOST_REQUIRE(!race_speed.finish({}, {}));
    BOOST_REQUIRE(!race_speed.running());
}

BOOST_AUTO_TEST_CASE(race_speed__running__3_of_3__false_expected_invocation)
{
    const std::pair<code, size_t> expected{ error::invalid_magic, 1 };
    std::pair<code, size_t> complete{};
    race_speed_t race_speed{};

    BOOST_REQUIRE(!race_speed.running());
    BOOST_REQUIRE(race_speed.start([&](code ec, size_t size) NOEXCEPT
    {
        complete = { ec, size };
    }));

    BOOST_REQUIRE(race_speed.running());
    BOOST_REQUIRE(race_speed.finish(expected.first, expected.second));
    BOOST_REQUIRE(race_speed.running());
    BOOST_REQUIRE(!race_speed.finish(error::accept_failed, 2));
    BOOST_REQUIRE(race_speed.running());
    BOOST_REQUIRE(!race_speed.finish(error::address_invalid, 3));
    BOOST_REQUIRE(!race_speed.running());
    BOOST_REQUIRE_EQUAL(complete.first, expected.first);
    BOOST_REQUIRE_EQUAL(complete.second, expected.second);
}

BOOST_AUTO_TEST_CASE(race_speed__running__4_of_3__false_expected_invocation)
{
    const std::pair<code, size_t> expected{ error::invalid_magic, 1 };
    std::pair<code, size_t> complete{};
    race_speed_t race_speed{};

    BOOST_REQUIRE(!race_speed.running());
    BOOST_REQUIRE(race_speed.start([&](code ec, size_t size) NOEXCEPT
    {
        complete = { ec, size };
    }));

    BOOST_REQUIRE(race_speed.running());
    BOOST_REQUIRE(race_speed.finish(expected.first, expected.second));
    BOOST_REQUIRE(!race_speed.finish(error::accept_failed, 2));
    BOOST_REQUIRE(!race_speed.finish(error::address_invalid, 3));
    BOOST_REQUIRE(!race_speed.running());
    BOOST_REQUIRE(!race_speed.finish(error::success, 4));
    BOOST_REQUIRE(!race_speed.running());
    BOOST_REQUIRE_EQUAL(complete.first, expected.first);
    BOOST_REQUIRE_EQUAL(complete.second, expected.second);
}

BOOST_AUTO_TEST_CASE(race_speed__finish__3_of_3__resources_deleted)
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
    race_speed<3, const code&, const destructor::ptr&> race_speed{};
    std::pair<bool, bool> complete{};

    // foo/bar captured/passed into handler.
    BOOST_REQUIRE(race_speed.start([=, &complete](const code&, const destructor::ptr& bar) NOEXCEPT
    {
        complete = { !foo->deleted_, !bar->deleted_ };
    }));

    BOOST_REQUIRE(race_speed.finish(error::success, foo));
    BOOST_REQUIRE(race_speed.running());
    BOOST_REQUIRE(!complete.first);
    BOOST_REQUIRE(!complete.second);

    // race_speed not finished, resources retained.
    foo.reset();
    BOOST_REQUIRE(!deleted);

    BOOST_REQUIRE(!race_speed.finish(error::success, {}));
    BOOST_REQUIRE(race_speed.running());
    BOOST_REQUIRE(!complete.first);
    BOOST_REQUIRE(!complete.second);

    BOOST_REQUIRE(!race_speed.finish(error::success, {}));
    BOOST_REQUIRE(!race_speed.running());
    BOOST_REQUIRE(complete.first);
    BOOST_REQUIRE(complete.second);

    // race_speed finished, resourced cleared.
    BOOST_REQUIRE(deleted);
}

BOOST_AUTO_TEST_SUITE_END()
