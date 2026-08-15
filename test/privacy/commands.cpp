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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(privacy_commands_tests)

using namespace network::privacy;
using namespace network::messages::peer;

BOOST_AUTO_TEST_CASE(privacy_commands__to_command__assigned__expected)
{
    BOOST_REQUIRE_EQUAL(to_command(identifiers::address), "addr");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::block), "block");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::compact_transactions), "blocktxn");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::compact_block), "cmpctblock");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::fee_filter), "feefilter");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::bloom_filter_add), "filteradd");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::bloom_filter_clear), "filterclear");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::bloom_filter_load), "filterload");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::get_blocks), "getblocks");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::get_compact_transactions), "getblocktxn");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::get_data), "getdata");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::get_headers), "getheaders");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::headers), "headers");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::inventory), "inv");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::memory_pool), "mempool");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::merkle_block), "merkleblock");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::not_found), "notfound");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::ping), "ping");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::pong), "pong");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::send_compact), "sendcmpct");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::transaction), "tx");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::get_client_filters), "getcfilters");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::client_filter), "cfilter");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::get_client_filter_headers), "getcfheaders");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::client_filter_headers), "cfheaders");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::get_client_filter_checkpoint), "getcfcheckpt");
    BOOST_REQUIRE_EQUAL(to_command(identifiers::client_filter_checkpoint), "cfcheckpt");
}

BOOST_AUTO_TEST_CASE(privacy_commands__to_command__unassigned__empty)
{
    BOOST_REQUIRE(to_command(identifiers::unassigned).empty());

    // bip155 addrv2 is assigned but not an implemented message type.
    BOOST_REQUIRE(to_command(identifiers::address_v2).empty());
    BOOST_REQUIRE(to_command(29).empty());
    BOOST_REQUIRE(to_command(255).empty());
}

BOOST_AUTO_TEST_CASE(privacy_commands__to_identifier__assigned__expected)
{
    BOOST_REQUIRE_EQUAL(to_identifier("addr"), identifiers::address);
    BOOST_REQUIRE_EQUAL(to_identifier("block"), identifiers::block);
    BOOST_REQUIRE_EQUAL(to_identifier("blocktxn"), identifiers::compact_transactions);
    BOOST_REQUIRE_EQUAL(to_identifier("cmpctblock"), identifiers::compact_block);
    BOOST_REQUIRE_EQUAL(to_identifier("feefilter"), identifiers::fee_filter);
    BOOST_REQUIRE_EQUAL(to_identifier("filteradd"), identifiers::bloom_filter_add);
    BOOST_REQUIRE_EQUAL(to_identifier("filterclear"), identifiers::bloom_filter_clear);
    BOOST_REQUIRE_EQUAL(to_identifier("filterload"), identifiers::bloom_filter_load);
    BOOST_REQUIRE_EQUAL(to_identifier("getblocks"), identifiers::get_blocks);
    BOOST_REQUIRE_EQUAL(to_identifier("getblocktxn"), identifiers::get_compact_transactions);
    BOOST_REQUIRE_EQUAL(to_identifier("getdata"), identifiers::get_data);
    BOOST_REQUIRE_EQUAL(to_identifier("getheaders"), identifiers::get_headers);
    BOOST_REQUIRE_EQUAL(to_identifier("headers"), identifiers::headers);
    BOOST_REQUIRE_EQUAL(to_identifier("inv"), identifiers::inventory);
    BOOST_REQUIRE_EQUAL(to_identifier("mempool"), identifiers::memory_pool);
    BOOST_REQUIRE_EQUAL(to_identifier("merkleblock"), identifiers::merkle_block);
    BOOST_REQUIRE_EQUAL(to_identifier("notfound"), identifiers::not_found);
    BOOST_REQUIRE_EQUAL(to_identifier("ping"), identifiers::ping);
    BOOST_REQUIRE_EQUAL(to_identifier("pong"), identifiers::pong);
    BOOST_REQUIRE_EQUAL(to_identifier("sendcmpct"), identifiers::send_compact);
    BOOST_REQUIRE_EQUAL(to_identifier("tx"), identifiers::transaction);
    BOOST_REQUIRE_EQUAL(to_identifier("getcfilters"), identifiers::get_client_filters);
    BOOST_REQUIRE_EQUAL(to_identifier("cfilter"), identifiers::client_filter);
    BOOST_REQUIRE_EQUAL(to_identifier("getcfheaders"), identifiers::get_client_filter_headers);
    BOOST_REQUIRE_EQUAL(to_identifier("cfheaders"), identifiers::client_filter_headers);
    BOOST_REQUIRE_EQUAL(to_identifier("getcfcheckpt"), identifiers::get_client_filter_checkpoint);
    BOOST_REQUIRE_EQUAL(to_identifier("cfcheckpt"), identifiers::client_filter_checkpoint);
}

BOOST_AUTO_TEST_CASE(privacy_commands__to_identifier__unassigned__zero)
{
    BOOST_REQUIRE_EQUAL(to_identifier("version"), identifiers::unassigned);
    BOOST_REQUIRE_EQUAL(to_identifier("verack"), identifiers::unassigned);
    BOOST_REQUIRE_EQUAL(to_identifier("sendaddrv2"), identifiers::unassigned);
    BOOST_REQUIRE_EQUAL(to_identifier("addrv2"), identifiers::unassigned);
    BOOST_REQUIRE_EQUAL(to_identifier(""), identifiers::unassigned);
}

BOOST_AUTO_TEST_SUITE_END()
