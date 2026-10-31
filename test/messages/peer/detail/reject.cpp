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

BC_AUTO_TEST_SUITE(p2p_reject_tests)

using namespace network::messages::peer;

BC_AUTO_TEST_CASE(reject__properties__always__expected)
{
    BC_REQUIRE_EQUAL(reject::command, "reject");
    constexpr auto index = messages::peer::registry::index_of<reject>();
    BC_REQUIRE_EQUAL(messages::peer::registry::commands().at(index), reject::command);
    BC_REQUIRE_EQUAL(reject::version_minimum, level::bip61);
    BC_REQUIRE_EQUAL(reject::version_maximum, level::maximum_protocol);
}

BC_AUTO_TEST_CASE(reject__size__default__expected)
{
    constexpr auto expected = variable_size(zero)
        + sizeof(uint8_t)
        + variable_size(zero)
        + zero;

    BC_REQUIRE_EQUAL(reject{}.size(level::canonical), expected);
}

struct accessor
  : public reject
{
    static bool is_chain_(const std::string& message) NOEXCEPT
    {
        return reject::is_chain(message);
    }
};

BC_AUTO_TEST_CASE(reject__is_chain__is__true)
{
    BC_REQUIRE(accessor::is_chain_(block::command));
    BC_REQUIRE(accessor::is_chain_(transaction::command));
}

BC_AUTO_TEST_CASE(reject__is_chain__is_not__false)
{
    BC_REQUIRE(!accessor::is_chain_(reject::command));
    BC_REQUIRE(!accessor::is_chain_(get_data::command));
    BC_REQUIRE(!accessor::is_chain_("foobar"));
}

BC_AUTO_TEST_CASE(reject__reason_to_byte__all__expected)
{
    BC_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::undefined), 0x00u);
    BC_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::malformed), 0x01);
    BC_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::invalid), 0x10);
    BC_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::obsolete), 0x11);
    BC_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::duplicate), 0x12);
    BC_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::nonstandard), 0x40);
    BC_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::dust), 0x41);
    BC_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::insufficient_fee), 0x42);
    BC_REQUIRE_EQUAL(reject::reason_to_byte(reject::reason_code::checkpoint), 0x43);
}

BC_AUTO_TEST_CASE(reject__byte_to_reason__all__expected)
{
    BC_REQUIRE(reject::byte_to_reason(0x00) == reject::reason_code::undefined);
    BC_REQUIRE(reject::byte_to_reason(0x01) == reject::reason_code::malformed);
    BC_REQUIRE(reject::byte_to_reason(0x10) == reject::reason_code::invalid);
    BC_REQUIRE(reject::byte_to_reason(0x11) == reject::reason_code::obsolete);
    BC_REQUIRE(reject::byte_to_reason(0x12) == reject::reason_code::duplicate);
    BC_REQUIRE(reject::byte_to_reason(0x40) == reject::reason_code::nonstandard);
    BC_REQUIRE(reject::byte_to_reason(0x41) == reject::reason_code::dust);
    BC_REQUIRE(reject::byte_to_reason(0x42) == reject::reason_code::insufficient_fee);
    BC_REQUIRE(reject::byte_to_reason(0x43) == reject::reason_code::checkpoint);
}

static const auto genesis_tx_hash = system::base16_hash("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");
static const auto dust_tx_payload = system::base16_chunk("0274784104647573743ba3edfd7a7b12b27ac72c3e67768f617fc81bc3888a51323a9fb8aa4b1e5e4a");
static const auto obsolete_version_payload = system::base16_chunk("0776657273696f6e11036f6c64");

BC_AUTO_TEST_CASE(reject__deserialize1__tx__expected)
{
    const auto message = reject::deserialize(level::bip61, dust_tx_payload);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->message, transaction::command);
    BC_REQUIRE(message->code == reject::reason_code::dust);
    BC_REQUIRE_EQUAL(message->reason, "dust");
    BC_REQUIRE_EQUAL(message->hash, genesis_tx_hash);
}

BC_AUTO_TEST_CASE(reject__deserialize1__tx_without_hash__null_hash)
{
    const auto data = system::base16_chunk("027478410464757374");
    const auto message = reject::deserialize(level::bip61, data);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->message, transaction::command);
    BC_REQUIRE_EQUAL(message->reason, "dust");
    BC_REQUIRE_EQUAL(message->hash, system::null_hash);
}

BC_AUTO_TEST_CASE(reject__deserialize1__tx_partial_hash__null_hash)
{
    const auto data = system::base16_chunk("0274784104647573743ba3edfd7a7b12b2");
    const auto message = reject::deserialize(level::bip61, data);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->message, transaction::command);
    BC_REQUIRE_EQUAL(message->reason, "dust");
    BC_REQUIRE_EQUAL(message->hash, system::null_hash);
}

BC_AUTO_TEST_CASE(reject__deserialize2__tx__exhausted)
{
    system::read::bytes::copy source(dust_tx_payload);
    const auto message = reject::deserialize(level::bip61, source);
    BC_REQUIRE(source);
    BC_REQUIRE(source.is_exhausted());
    BC_REQUIRE_EQUAL(message.hash, genesis_tx_hash);
}

BC_AUTO_TEST_CASE(reject__deserialize1__version__null_hash)
{
    const auto message = reject::deserialize(level::bip61, obsolete_version_payload);
    BC_REQUIRE(message);
    BC_REQUIRE_EQUAL(message->message, version::command);
    BC_REQUIRE(message->code == reject::reason_code::obsolete);
    BC_REQUIRE_EQUAL(message->reason, "old");
    BC_REQUIRE_EQUAL(message->hash, system::null_hash);
}

BC_AUTO_TEST_CASE(reject__deserialize1__unknown_code__undefined)
{
    const auto data = system::base16_chunk("0776657273696f6e99036f6c64");
    const auto message = reject::deserialize(level::bip61, data);
    BC_REQUIRE(message);
    BC_REQUIRE(message->code == reject::reason_code::undefined);
}

BC_AUTO_TEST_CASE(reject__deserialize1__underflow__nullptr)
{
    const auto data = system::base16_chunk("027478");
    BC_REQUIRE(!reject::deserialize(level::bip61, data));
}

BC_AUTO_TEST_CASE(reject__deserialize2__version__exhausted)
{
    system::read::bytes::copy source(obsolete_version_payload);
    const auto message = reject::deserialize(level::bip61, source);
    BC_REQUIRE(source);
    BC_REQUIRE(source.is_exhausted());
    BC_REQUIRE_EQUAL(message.message, version::command);
}

BC_AUTO_TEST_CASE(reject__serialize1__tx__expected)
{
    const reject message{ transaction::command, reject::reason_code::dust, "dust", genesis_tx_hash };
    BC_REQUIRE_EQUAL(message.size(level::bip61), dust_tx_payload.size());
    system::data_chunk data(message.size(level::bip61));
    BC_REQUIRE(message.serialize(level::bip61, data));
    BC_REQUIRE_EQUAL(data, dust_tx_payload);
}

BC_AUTO_TEST_CASE(reject__serialize1__version__hash_omitted)
{
    const reject message{ version::command, reject::reason_code::obsolete, "old", genesis_tx_hash };
    BC_REQUIRE_EQUAL(message.size(level::bip61), obsolete_version_payload.size());
    system::data_chunk data(message.size(level::bip61));
    BC_REQUIRE(message.serialize(level::bip61, data));
    BC_REQUIRE_EQUAL(data, obsolete_version_payload);
}

BC_AUTO_TEST_SUITE_END()
