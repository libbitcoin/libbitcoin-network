/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(track_tests)

class tracked
  : tracker<tracked>
{
public:
    tracked(const logger& log) NOEXCEPT
      : tracker<tracked>(log)
    {
    }

    bool method() const NOEXCEPT
    {
        return true;
    };
};

BOOST_AUTO_TEST_CASE(track__construct__always__compiles)
{
    const logger log{};
    tracked foo{ log };
    BOOST_REQUIRE(foo.method());
}

BOOST_AUTO_TEST_SUITE_END()
