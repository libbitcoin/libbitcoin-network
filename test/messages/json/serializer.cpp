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

#include <variant>

BOOST_AUTO_TEST_SUITE(serializer_tests)

using namespace network::json;

BOOST_AUTO_TEST_CASE(serializer__serialize__request__expected)
{
    const string_t text
    {
        R"({)"
            R"("jsonrpc":"2.0",)"
            R"("method":"random",)"
            R"("id":-42,)"
            R"("params":)"
            R"({)"
                R"("array":[A],)"
                R"("false":false,)"
                R"("foo":"bar",)"
                R"("null":null,)"
                R"("number":42,)"
                R"("object":{O},)"
                R"("true":true)"
            R"(})"
        R"(})"
    };

    parser parse{};
    BOOST_REQUIRE_EQUAL(parse.write(text), text.size());
    BOOST_REQUIRE(parse);

    // params are sorted by serializer, so must be above as well.
    BOOST_REQUIRE_EQUAL(serialize(parse.get()), text);
}

BOOST_AUTO_TEST_SUITE_END()
