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

BOOST_AUTO_TEST_SUITE(http_body_writer_tests)

using namespace network::http;

struct accessor
  : public body::writer
{
    using base = body::writer;
    using base::writer;
    using base::to_writer;
};

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__undefined__constructs_empty_writer)
{
    header<false, fields> header{};
    variant_payload payload{};
    ///payload.inner = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, payload);
    BOOST_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__empty__constructs_empty_writer)
{
    header<false, fields> header{};
    variant_payload payload{};
    payload.inner = empty_body::value_type{};
    const auto variant = accessor::to_writer(header, payload);
    BOOST_REQUIRE(std::holds_alternative<empty_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__json__constructs_json_writer)
{
    header<false, fields> header{};
    variant_payload payload{};
    payload.inner = json_body::value_type{};
    const auto variant = accessor::to_writer(header, payload);
    BOOST_REQUIRE(std::holds_alternative<json_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__data__constructs_data_writer)
{
    header<false, fields> header{};
    variant_payload payload{};
    payload.inner = data_body::value_type{};
    const auto variant = accessor::to_writer(header, payload);
    BOOST_REQUIRE(std::holds_alternative<data_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__file__constructs_file_writer)
{
    header<false, fields> header{};
    variant_payload payload{};
    payload.inner = file_body::value_type{};
    const auto variant = accessor::to_writer(header, payload);
    BOOST_REQUIRE(std::holds_alternative<file_writer>(variant));
}

BOOST_AUTO_TEST_CASE(http_body_writer__to_writer__string__constructs_string_writer)
{
    header<false, fields> header{};
    variant_payload payload{};
    payload.inner = string_body::value_type{};
    const auto variant = accessor::to_writer(header, payload);
    BOOST_REQUIRE(std::holds_alternative<string_writer>(variant));
}

BOOST_AUTO_TEST_SUITE_END()
