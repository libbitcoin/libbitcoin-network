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

BC_AUTO_TEST_SUITE(http_method_tests)

using namespace network::http::method;

BC_AUTO_TEST_CASE(method__static_method__always__equals_verb)
{
    BC_REQUIRE(get::method == http::verb::get);
    BC_REQUIRE(head::method == http::verb::head);
    BC_REQUIRE(post::method == http::verb::post);
    BC_REQUIRE(put::method == http::verb::put);
    BC_REQUIRE(delete_::method == http::verb::delete_);
    BC_REQUIRE(trace::method == http::verb::trace);
    BC_REQUIRE(options::method == http::verb::options);
    BC_REQUIRE(connect::method == http::verb::connect);
    BC_REQUIRE(unknown::method == http::verb::unknown);
    BC_REQUIRE(websocket::method == http::verb::websocket);
}

BC_AUTO_TEST_SUITE_END()
