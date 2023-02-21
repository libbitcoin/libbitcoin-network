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

BOOST_AUTO_TEST_SUITE(speed_racer_tests)

using speed_racer_t = speed_racer<3, const code&, size_t>;

BOOST_AUTO_TEST_CASE(speed_racer__running__unstarted__false)
{
    speed_racer_t speed_racer{};
    BOOST_REQUIRE(!speed_racer.running());
}

BOOST_AUTO_TEST_CASE(speed_racer__start__unstarted__true_running)
{
    speed_racer_t speed_racer{};
    BOOST_REQUIRE(speed_racer.start([&](code, size_t) NOEXCEPT {}));
    BOOST_REQUIRE(speed_racer.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(speed_racer.finish({}, {}));
    BOOST_REQUIRE(speed_racer.finish({}, {}));
    BOOST_REQUIRE(speed_racer.finish({}, {}));
    BOOST_REQUIRE(!speed_racer.running());
}

BOOST_AUTO_TEST_CASE(speed_racer__start__started__false_running)
{
    speed_racer_t speed_racer{};
    BOOST_REQUIRE(speed_racer.start([&](code, size_t) NOEXCEPT {}));
    BOOST_REQUIRE(!speed_racer.start([&](code, size_t) NOEXCEPT{}));
    BOOST_REQUIRE(speed_racer.running());

    // Avoid running at destruct assertion.
    BOOST_REQUIRE(speed_racer.finish({}, {}));
    BOOST_REQUIRE(speed_racer.finish({}, {}));
    BOOST_REQUIRE(speed_racer.finish({}, {}));
    BOOST_REQUIRE(!speed_racer.running());
}

BOOST_AUTO_TEST_CASE(speed_racer__running__3_of_3__false_expected_invocation)
{
    const std::pair<code, size_t> expected{ error::invalid_magic, 1 };
    std::pair<code, size_t> complete{};
    speed_racer_t speed_racer{};

    BOOST_REQUIRE(!speed_racer.running());
    BOOST_REQUIRE(speed_racer.start([&](code ec, size_t size) NOEXCEPT
    {
        complete = { ec, size };
    }));

    BOOST_REQUIRE(speed_racer.running());
    BOOST_REQUIRE(speed_racer.finish(expected.first, expected.second));
    BOOST_REQUIRE(speed_racer.running());
    BOOST_REQUIRE(speed_racer.finish(error::accept_failed, 2));
    BOOST_REQUIRE(speed_racer.running());
    BOOST_REQUIRE(speed_racer.finish(error::address_invalid, 3));
    BOOST_REQUIRE(!speed_racer.running());
    BOOST_REQUIRE_EQUAL(complete.first, expected.first);
    BOOST_REQUIRE_EQUAL(complete.second, expected.second);
}

BOOST_AUTO_TEST_CASE(speed_racer__running__4_of_3__false_expected_invocation)
{
    const std::pair<code, size_t> expected{ error::invalid_magic, 1 };
    std::pair<code, size_t> complete{};
    speed_racer_t speed_racer{};

    BOOST_REQUIRE(!speed_racer.running());
    BOOST_REQUIRE(speed_racer.start([&](code ec, size_t size) NOEXCEPT
    {
        complete = { ec, size };
    }));

    BOOST_REQUIRE(speed_racer.running());
    BOOST_REQUIRE(speed_racer.finish(expected.first, expected.second));
    BOOST_REQUIRE(speed_racer.finish(error::accept_failed, 2));
    BOOST_REQUIRE(speed_racer.finish(error::address_invalid, 3));
    BOOST_REQUIRE(!speed_racer.running());
    BOOST_REQUIRE(!speed_racer.finish(error::success, 4));
    BOOST_REQUIRE(!speed_racer.running());
    BOOST_REQUIRE_EQUAL(complete.first, expected.first);
    BOOST_REQUIRE_EQUAL(complete.second, expected.second);
}

BOOST_AUTO_TEST_CASE(speed_racer__finish__3_of_3__resources_deleted)
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
    speed_racer<3, const code&, const destructor::ptr&> speed_racer{};

    BOOST_REQUIRE(speed_racer.start([=](const code&, const destructor::ptr& bar)
    {
        // foo captured in handler.
        BOOST_REQUIRE(!foo->deleted_);

        // foo captured in first args and passed as bar.
        BOOST_REQUIRE(!bar->deleted_);
    }));

    BOOST_REQUIRE(speed_racer.finish(error::success, foo));
    BOOST_REQUIRE(speed_racer.running());

    // speed_racer not finished, resources retained.
    foo.reset();
    BOOST_REQUIRE(!deleted);

    BOOST_REQUIRE(speed_racer.finish(error::success, {}));
    BOOST_REQUIRE(speed_racer.running());

    BOOST_REQUIRE(speed_racer.finish(error::success, {}));
    BOOST_REQUIRE(!speed_racer.running());

    // speed_racer finished, resourced cleared.
    BOOST_REQUIRE(deleted);
}

BOOST_AUTO_TEST_SUITE_END()
