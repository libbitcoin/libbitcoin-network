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
#include "../../../test.hpp"

BOOST_AUTO_TEST_SUITE(p2p_reject_tests)

using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(reject__properties__always__expected)
{
    BOOST_REQUIRE_EQUAL(reject::command, "reject");
    constexpr auto index = messages::peer::registry::index_of<reject>();
    BOOST_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), reject::command);
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

static const auto genesis_tx_hash = system::base16_hash("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
static const auto dust_tx_payload = system::base16_chunk("0274784104647573743ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a");
static const auto obsolete_version_payload = system::base16_chunk("0776657273696f6e11036f6c64");

BOOST_AUTO_TEST_CASE(reject__deserialize1__tx__expected)
{
    const auto message = reject::deserialize(level::bip61, dust_tx_payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->message, transaction::command);
    BOOST_REQUIRE(message->code == reject::reason_code::dust);
    BOOST_REQUIRE_EQUAL(message->reason, "dust");
}

BOOST_AUTO_TEST_CASE(reject__deserialize1__version__null_hash)
{
    const auto message = reject::deserialize(level::bip61, obsolete_version_payload);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE_EQUAL(message->message, version::command);
    BOOST_REQUIRE(message->code == reject::reason_code::obsolete);
    BOOST_REQUIRE_EQUAL(message->reason, "old");
    BOOST_REQUIRE_EQUAL(message->hash, system::null_hash);
}

BOOST_AUTO_TEST_CASE(reject__deserialize1__unknown_code__undefined)
{
    const auto data = system::base16_chunk("0776657273696f6e99036f6c64");
    const auto message = reject::deserialize(level::bip61, data);
    BOOST_REQUIRE(message);
    BOOST_REQUIRE(message->code == reject::reason_code::undefined);
}

BOOST_AUTO_TEST_CASE(reject__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("027478");
    BOOST_REQUIRE(!reject::deserialize(level::bip61, data));
}

BOOST_AUTO_TEST_CASE(reject__deserialize2__version__exhausted)
{
    system::read::bytes::copy source(obsolete_version_payload);
    const auto message = reject::deserialize(level::bip61, source);
    BOOST_REQUIRE(source);
    BOOST_REQUIRE(source.is_exhausted());
    BOOST_REQUIRE_EQUAL(message.message, version::command);
}

BOOST_AUTO_TEST_CASE(reject__serialize1__tx__expected)
{
    const reject message{ transaction::command, reject::reason_code::dust, "dust", genesis_tx_hash };
    BOOST_REQUIRE_EQUAL(message.size(level::bip61), dust_tx_payload.size());
    system::data_chunk data(message.size(level::bip61));
    BOOST_REQUIRE(message.serialize(level::bip61, data));
    BOOST_REQUIRE_EQUAL(data, dust_tx_payload);
}

BOOST_AUTO_TEST_CASE(reject__serialize1__version__hash_omitted)
{
    const reject message{ version::command, reject::reason_code::obsolete, "old", genesis_tx_hash };
    BOOST_REQUIRE_EQUAL(message.size(level::bip61), obsolete_version_payload.size());
    system::data_chunk data(message.size(level::bip61));
    BOOST_REQUIRE(message.serialize(level::bip61, data));
    BOOST_REQUIRE_EQUAL(data, obsolete_version_payload);
}

BOOST_AUTO_TEST_SUITE_END()
