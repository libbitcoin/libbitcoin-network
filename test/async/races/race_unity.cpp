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

BC_AUTO_TEST_SUITE(race_unity_tests)

using race_unity_t = race_unity<const code&, size_t>;

BC_AUTO_TEST_CASE(race_unity__running__empty__false)
{
    race_unity_t race_unity{ 0 };
    BC_REQUIRE(!race_unity.running());
}

BC_AUTO_TEST_CASE(race_unity__running__unstarted__false)
{
    race_unity_t race_unity{ 2 };
    BC_REQUIRE(!race_unity.running());
}

BC_AUTO_TEST_CASE(race_unity__start__unstarted__true_running)
{
    race_unity_t race_unity{ 3 };
    BC_REQUIRE(race_unity.start([&](code, size_t) NOEXCEPT {}));
    BC_REQUIRE(race_unity.running());

    // Avoid running at destruct assertion.
    BC_REQUIRE(!race_unity.finish({}, {}));
    BC_REQUIRE(!race_unity.finish({}, {}));
    BC_REQUIRE(race_unity.finish({}, {}));
    BC_REQUIRE(!race_unity.running());
}

BC_AUTO_TEST_CASE(race_unity__start__started__false_running)
{
    race_unity_t race_unity{ 3 };
    BC_REQUIRE(race_unity.start([&](code, size_t) NOEXCEPT {}));
    BC_REQUIRE(!race_unity.start([&](code, size_t) NOEXCEPT {}));
    BC_REQUIRE(race_unity.running());

    // Avoid running at destruct assertion.
    BC_REQUIRE(!race_unity.finish({}, {}));
    BC_REQUIRE(!race_unity.finish({}, {}));
    BC_REQUIRE(race_unity.finish({}, {}));
    BC_REQUIRE(!race_unity.running());
}

BC_AUTO_TEST_CASE(race_unity__running__3_of_3__false_expected_invocation)
{
    const std::pair<code, size_t> expected{ error::invalid_magic, 3 };
    std::pair<code, size_t> complete{};
    race_unity_t race_unity{ 3 };

    BC_REQUIRE(!race_unity.running());
    BC_REQUIRE(race_unity.start([&](code ec, size_t size) NOEXCEPT
    {
        complete = { ec, size };
    }));

    BC_REQUIRE(race_unity.running());
    BC_REQUIRE(!race_unity.finish(expected.first, expected.second));
    BC_REQUIRE(race_unity.running());
    BC_REQUIRE(!race_unity.finish(error::address_invalid, 1));
    BC_REQUIRE(race_unity.running());
    BC_REQUIRE(!race_unity.finish(error::accept_failed, 2));
    BC_REQUIRE(!race_unity.running());
    BC_REQUIRE_EQUAL(complete.first, expected.first);
    BC_REQUIRE_EQUAL(complete.second, expected.second);
}

BC_AUTO_TEST_CASE(race_unity__running__4_of_3__false_expected_invocation)
{
    const std::pair<code, size_t> expected{ error::invalid_magic, 2 };
    std::pair<code, size_t> complete{};
    race_unity_t race_unity{ 3 };

    BC_REQUIRE(!race_unity.running());
    BC_REQUIRE(race_unity.start([&](code ec, size_t size) NOEXCEPT
    {
        complete = { ec, size };
    }));

    BC_REQUIRE(race_unity.running());
    BC_REQUIRE(!race_unity.finish(error::success, 1));
    BC_REQUIRE(!race_unity.finish(expected.first, expected.second));
    BC_REQUIRE(!race_unity.finish(error::address_invalid, 3));
    BC_REQUIRE(!race_unity.running());
    BC_REQUIRE(!race_unity.finish(error::success, 4));
    BC_REQUIRE(!race_unity.running());
    BC_REQUIRE_EQUAL(complete.first, expected.first);
    BC_REQUIRE_EQUAL(complete.second, expected.second);
}

BC_AUTO_TEST_CASE(race_unity__finish__3_of_3__resources_deleted)
{
    struct destructor
    {
        using ptr = std::shared_ptr<destructor>;
        destructor(bool& deleted) NOEXCEPT : deleted_(deleted) {}
        ~destructor() NOEXCEPT { deleted_ = true; }
        bool& deleted_;
    };

    const code expected{ error::invalid_magic };
    bool deleted{ false };
    auto foo = std::make_shared<destructor>(deleted);
    race_unity<const code&, const destructor::ptr&> race_unity{ 3 };
    std::pair<bool, bool> complete{};
    code result{ error::success };

    // foo/bar captured/passed into handler.
    BC_REQUIRE(race_unity.start([=, &complete, &result](const code& ec, const destructor::ptr& bar) NOEXCEPT
    {
        result = ec;
        complete = { !foo->deleted_, !bar->deleted_ };
    }));

    // First finish is failer, captures foo/ec.
    BC_REQUIRE(!race_unity.finish(expected, foo));
    BC_REQUIRE(race_unity.running());
    BC_REQUIRE(!complete.first);
    BC_REQUIRE(!complete.second);
    BC_REQUIRE(!result);

    // race_unity not finished, resources retained.
    foo.reset();
    BC_REQUIRE(!deleted);

    // race_unity not finished, resources retained.
    BC_REQUIRE(!race_unity.finish(error::success, {}));
    BC_REQUIRE(race_unity.running());
    BC_REQUIRE(!complete.first);
    BC_REQUIRE(!complete.second);
    BC_REQUIRE(!result);
    BC_REQUIRE(!deleted);

    // race_unity finished (invoked with failer), resourced cleared.
    BC_REQUIRE(!race_unity.finish(error::success, {}));
    BC_REQUIRE(!race_unity.running());
    BC_REQUIRE(complete.first);
    BC_REQUIRE(complete.second);
    BC_REQUIRE_EQUAL(result, expected);
    BC_REQUIRE(deleted);
}

BC_AUTO_TEST_SUITE_END()
