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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(reject_tests)

using namespace bc::network::messages;

BOOST_AUTO_TEST_CASE(reject__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(reject::command, "reject");
    BOOST_REQUIRE(reject::id == identifier::reject);
    BOOST_REQUIRE_EQUAL(reject::version_minimum, level::bip61);
    BOOST_REQUIRE_EQUAL(reject::version_maximum, level::maximum_protocol);
}

BOOST_AUTO_TEST_CASE(reject__size__default__expected)
{
    constexpr auto expected = variable_size(zero)
        + sizeof(uint8_t)
        + variable_size(zero)
        + zero;

    BOOST_REQUIRE_EQUAL(reject{}.size(level::canonical), expected);
}

struct accessor
  : public reject
{
    static bool is_chain_(const std::string& message) NOEXCEPT
    {
        return reject::is_chain(message);
    }
};

BOOST_AUTO_TEST_CASE(reject__is_chain__is__true)
{
    BOOST_REQUIRE(accessor::is_chain_(block::command));
    BOOST_REQUIRE(accessor::is_chain_(transaction::command));
}

BOOST_AUTO_TEST_CASE(reject__is_chain__is_not__false)
{
    BOOST_REQUIRE(!accessor::is_chain_(reject::command));
    BOOST_REQUIRE(!accessor::is_chain_(get_data::command));
    BOOST_REQUIRE(!accessor::is_chain_("foobar"));
}

BOOST_AUTO_TEST_CASE(reject__reason_to_byte__all__expected)
{
    BOOST_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::undefined), 0x00u);
    BOOST_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::malformed), 0x01);
    BOOST_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::invalid), 0x10);
    BOOST_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::obsolete), 0x11);
    BOOST_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::duplicate), 0x12);
    BOOST_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::nonstandard), 0x40);
    BOOST_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::dust), 0x41);
    BOOST_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::insufficient_fee), 0x42);
    BOOST_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::checkpoint), 0x43);
}

BOOST_AUTO_TEST_CASE(reject__byte_to_reason__all__expected)
{
    BOOST_REQUIRE(reject::byte_to_reason(0x00) == reject::reason_code::undefined);
    BOOST_REQUIRE(reject::byte_to_reason(0x01) == reject::reason_code::malformed);
    BOOST_REQUIRE(reject::byte_to_reason(0x10) == reject::reason_code::invalid);
    BOOST_REQUIRE(reject::byte_to_reason(0x11) == reject::reason_code::obsolete);
    BOOST_REQUIRE(reject::byte_to_reason(0x12) == reject::reason_code::duplicate);
    BOOST_REQUIRE(reject::byte_to_reason(0x40) == reject::reason_code::nonstandard);
    BOOST_REQUIRE(reject::byte_to_reason(0x41) == reject::reason_code::dust);
    BOOST_REQUIRE(reject::byte_to_reason(0x42) == reject::reason_code::insufficient_fee);
    BOOST_REQUIRE(reject::byte_to_reason(0x43) == reject::reason_code::checkpoint);
}

BOOST_AUTO_TEST_SUITE_END()
