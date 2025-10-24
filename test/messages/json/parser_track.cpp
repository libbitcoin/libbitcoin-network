/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

BOOST_AUTO_TEST_SUITE(parser_track_tests)

using namespace network::json;

BOOST_AUTO_TEST_CASE(parser_track__initial_state__empty_true_comma_false)
{
    parser_track track{};

    BOOST_CHECK(track.allow_add());
    BOOST_CHECK(!track.allow_delimiter());
    BOOST_CHECK(track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__reset__always__empty_true_comma_false)
{
    parser_track track{};
    track.add();
    track.delimiter();
    track.reset();

    BOOST_CHECK(track.allow_add());
    BOOST_CHECK(!track.allow_delimiter());
    BOOST_CHECK(track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__add__after_reset__empty_false_comma_false)
{
    parser_track track{};
    track.reset();
    track.add();

    BOOST_CHECK(!track.allow_add());
    BOOST_CHECK(track.allow_delimiter());
    BOOST_CHECK(track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__delimiter__after_add__comma_true)
{
    parser_track track{};
    track.reset();
    track.add();
    track.delimiter();

    BOOST_CHECK(track.allow_add());
    BOOST_CHECK(!track.allow_delimiter());
    BOOST_CHECK(!track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_tracker__add__after_delimiter__empty_false_comma_false)
{
    parser_track track{};
    track.add();
    track.delimiter();
    track.add();

    BOOST_CHECK(!track.allow_add());
    BOOST_CHECK(track.allow_delimiter());
    BOOST_CHECK(track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__allow_add__empty__true)
{
    parser_track track{};
    track.reset();

    BOOST_CHECK(track.allow_add());
}

BOOST_AUTO_TEST_CASE(parser_track__allow_add__after_add__false)
{
    parser_track track{};
    track.add();

    BOOST_CHECK(!track.allow_add());
}

BOOST_AUTO_TEST_CASE(parser_track__allow_add__after_comma__true)
{
    parser_track track{};
    track.add();
    track.delimiter();

    BOOST_CHECK(track.allow_add());
}

BOOST_AUTO_TEST_CASE(parser_track__allow_delimiter__empty__false)
{
    parser_track track{};

    BOOST_CHECK(!track.allow_delimiter());
}

BOOST_AUTO_TEST_CASE(parser_track__allow_delimiter__after_add__true)
{
    parser_track track{};
    track.add();

    BOOST_CHECK(track.allow_delimiter());
}

BOOST_AUTO_TEST_CASE(parser_track__allow_delimiter__after_comma__false)
{
    parser_track track{};
    track.add();
    track.delimiter();

    BOOST_CHECK(!track.allow_delimiter());
}

BOOST_AUTO_TEST_CASE(parser_track__allow_close__empty__true)
{
    parser_track track{};

    BOOST_CHECK(track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__allow_close__after_add__true)
{
    parser_track track{};
    track.add();

    BOOST_CHECK(track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__allow_close__after_comma__false)
{
    parser_track track{};
    track.add();
    track.delimiter();

    BOOST_CHECK(!track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__sequence__add_comma_add__correct_states)
{
    parser_track track{};

    // First element
    BOOST_CHECK(track.allow_add());
    track.add();

    // Comma
    BOOST_CHECK(track.allow_delimiter());
    track.delimiter();

    // Second element
    BOOST_CHECK(track.allow_add());
    track.add();

    // Close
    BOOST_CHECK(track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__sequence__comma_at_start__disallowed)
{
    parser_track track{};

    BOOST_CHECK(!track.allow_delimiter());
}

BOOST_AUTO_TEST_CASE(parser_track__sequence__double_comma__disallowed)
{
    parser_track track{};
    track.add();
    track.delimiter();

    BOOST_CHECK(!track.allow_delimiter());
}

BOOST_AUTO_TEST_CASE(parser_track__sequence__close_after_comma__disallowed)
{
    parser_track track{};
    track.add();
    track.delimiter();

    BOOST_CHECK(!track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__sequence__close_after_add__allowed)
{
    parser_track track{};
    track.add();

    BOOST_CHECK(track.allow_close());
}

BOOST_AUTO_TEST_CASE(parser_track__sequence__empty_container_close__allowed)
{
    parser_track track{};

    BOOST_CHECK(track.allow_close());
}

BOOST_AUTO_TEST_SUITE_END()
