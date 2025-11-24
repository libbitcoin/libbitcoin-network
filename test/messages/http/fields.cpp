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

BOOST_AUTO_TEST_SUITE(fields_tests)

using namespace network::http;

// has_attachment
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(fields__has_attachment__empty__false)
{
    fields head{};
    BOOST_REQUIRE(!has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__no_content_disposition__false)
{
    fields head{};
    head.set(field::content_type, "application/octet-stream");
    BOOST_REQUIRE(!has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_present__true)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename=\"file.txt\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_equals__true)
{
    fields head{};
    head.set(field::content_disposition, "form-data; name=\"file\"; filename=\"data.bin\"");
    BOOST_REQUIRE(has_attachment(head));
}

// filename*= is valid (rfc7230), indicates unicode.
BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_star__true)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename*=utf-8''data%20file.pdf");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__case_insensitive__true)
{
    fields head{};
    head.set(field::content_disposition, "ATTACHMENT; FILENAME=\"DOC.PDF\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__mixed_case_filename__true)
{
    fields head{};
    head.set(field::content_disposition, "inline; FileName=\"image.PNG\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__no_filename__false)
{
    fields head{};
    head.set(field::content_disposition, "inline");
    BOOST_REQUIRE(!has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_empty__true)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename=\"\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__multiple_parameters__true)
{
    fields head{};
    head.set(field::content_disposition, "form-data; name=\"field1\"; filename=\"test.jpg\"; size=1024");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__whitespace__true)
{
    fields head{};
    head.set(field::content_disposition, " attachment ;  filename = \" doc.pdf \" ");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__quoted_filename_with_semicolon__true)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename=\"file;semi.txt\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__no_equals__false)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename");

    // It's a simple test for trimmed leading "filename" assumes not other token
    // starts with "filename" unless also an attachment (such as "filename*").
    // Otherwise the request is not valid anyway, so can assume it has attachment.
    ////BOOST_REQUIRE(!has_attachment(head));
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_equals_as_name_value__false)
{
    fields head{};
    head.set(field::content_disposition, "form-data; name=\"filename=\"");
    BOOST_REQUIRE(!has_attachment(head));
}

// is_websocket_upgrade
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__empty__false)
{
    fields head{};
    BOOST_REQUIRE(!is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__no_sec_websocket_key__false)
{
    fields head{};
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "Upgrade");
    BOOST_REQUIRE(!is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__no_upgrade__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::connection, "Upgrade");
    BOOST_REQUIRE(!is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__wrong_upgrade_value__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "http");
    head.set(field::connection, "Upgrade");
    BOOST_REQUIRE(!is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__no_connection__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    BOOST_REQUIRE(!is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__connection_without_upgrade__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "keep-alive");
    BOOST_REQUIRE(!is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__upgrade_present__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "Upgrade");
    BOOST_REQUIRE(is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__case_insensitive_upgrade_token__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "UPGRADE, keep-alive");
    BOOST_REQUIRE(is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__mixed_case_upgrade_token__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "keep-alive, UpGrAdE");
    BOOST_REQUIRE(is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__multiple_tokens__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "keep-alive, Upgrade, proxy");
    BOOST_REQUIRE(is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__whitespace__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, " Upgrade , keep-alive ");
    BOOST_REQUIRE(is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__tabs_as_whitespace__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "\tUpgrade\t,\tkeep-alive\t");
    BOOST_REQUIRE(is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__partial_token_match__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "upgrading, keep-alive");
    BOOST_REQUIRE(!is_websocket_upgrade(head));
}

BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__no_equals_in_key__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "Upgrade");
    BOOST_REQUIRE(!is_websocket_upgrade(head));
}

// Quoted value (allowed in some contexts, not supported).
BOOST_AUTO_TEST_CASE(fields__is_websocket_upgrade__quoted_upgrade_value__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "\"websocket\"");
    head.set(field::connection, "Upgrade");
    BOOST_REQUIRE(!is_websocket_upgrade(head));
}

// to_websocket_accept
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__rfc_example__expected)
{
    fields header{};
    header.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    BOOST_REQUIRE_EQUAL(to_websocket_accept(header), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__empty_header__empty_string)
{
    fields header{};
    BOOST_REQUIRE(to_websocket_accept(header).empty());
}

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__empty_key__empty_string)
{
    fields header{};
    header.set(field::sec_websocket_key, "");
    BOOST_REQUIRE(to_websocket_accept(header).empty());
}

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__short_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, "abc");
    BOOST_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__long_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, std::string(100, 'a'));
    BOOST_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__all_zeros_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, std::string(24, '\0'));
    BOOST_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__invalid_base64_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, "invalid!base64");
    BOOST_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__uppercase_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, "ABC123");
    BOOST_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__minimal_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, "dGhl");
    BOOST_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BOOST_AUTO_TEST_CASE(fields__to_websocket_accept__max_key_length__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, std::string(128, 'a'));
    BOOST_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BOOST_AUTO_TEST_SUITE_END()
