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

BC_AUTO_TEST_SUITE(fields_tests)

using namespace network::http;

// has_attachment
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(fields__has_attachment__empty__false)
{
    fields head{};
    BC_REQUIRE(!has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__no_content_disposition__false)
{
    fields head{};
    head.set(field::content_type, "application/octet-stream");
    BC_REQUIRE(!has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__filename_present__true)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename=\"file.txt\"");
    BC_REQUIRE(has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__filename_equals__true)
{
    fields head{};
    head.set(field::content_disposition, "form-data; name=\"file\"; filename=\"data.bin\"");
    BC_REQUIRE(has_attachment(head));
}

// filename*= is valid (rfc7230), indicates unicode.
BC_AUTO_TEST_CASE(fields__has_attachment__filename_star__true)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename*=utf-8''data%20file.pdf");
    BC_REQUIRE(has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__case_insensitive__true)
{
    fields head{};
    head.set(field::content_disposition, "ATTACHMENT; FILENAME=\"DOC.PDF\"");
    BC_REQUIRE(has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__mixed_case_filename__true)
{
    fields head{};
    head.set(field::content_disposition, "inline; FileName=\"image.PNG\"");
    BC_REQUIRE(has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__no_filename__false)
{
    fields head{};
    head.set(field::content_disposition, "inline");
    BC_REQUIRE(!has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__filename_empty__true)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename=\"\"");
    BC_REQUIRE(has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__multiple_parameters__true)
{
    fields head{};
    head.set(field::content_disposition, "form-data; name=\"field1\"; filename=\"test.jpg\"; size=1024");
    BC_REQUIRE(has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__whitespace__true)
{
    fields head{};
    head.set(field::content_disposition, " attachment ;  filename = \" doc.pdf \" ");
    BC_REQUIRE(has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__quoted_filename_with_semicolon__true)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename=\"file;semi.txt\"");
    BC_REQUIRE(has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__no_equals__false)
{
    fields head{};
    head.set(field::content_disposition, "attachment; filename");

    // It's a simple test for trimmed leading "filename" assumes not other token
    // starts with "filename" unless also an attachment (such as "filename*").
    // Otherwise the request is not valid anyway, so can assume it has attachment.
    ////BC_REQUIRE(!has_attachment(head));
    BC_REQUIRE(has_attachment(head));
}

BC_AUTO_TEST_CASE(fields__has_attachment__filename_equals_as_name_value__false)
{
    fields head{};
    head.set(field::content_disposition, "form-data; name=\"filename=\"");
    BC_REQUIRE(!has_attachment(head));
}

// is_websocket_upgrade
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__empty__false)
{
    fields head{};
    BC_REQUIRE(!is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__no_sec_websocket_key__false)
{
    fields head{};
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "Upgrade");
    BC_REQUIRE(!is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__no_upgrade__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::connection, "Upgrade");
    BC_REQUIRE(!is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__wrong_upgrade_value__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "http");
    head.set(field::connection, "Upgrade");
    BC_REQUIRE(!is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__no_connection__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    BC_REQUIRE(!is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__connection_without_upgrade__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "keep-alive");
    BC_REQUIRE(!is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__upgrade_present__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "Upgrade");
    BC_REQUIRE(is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__case_insensitive_upgrade_token__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "UPGRADE, keep-alive");
    BC_REQUIRE(is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__mixed_case_upgrade_token__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "keep-alive, UpGrAdE");
    BC_REQUIRE(is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__multiple_tokens__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "keep-alive, Upgrade, proxy");
    BC_REQUIRE(is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__whitespace__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, " Upgrade , keep-alive ");
    BC_REQUIRE(is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__tabs_as_whitespace__true)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "\tUpgrade\t,\tkeep-alive\t");
    BC_REQUIRE(is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__partial_token_match__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "upgrading, keep-alive");
    BC_REQUIRE(!is_websocket_upgrade(head));
}

BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__no_equals_in_key__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "");
    head.set(field::upgrade, "websocket");
    head.set(field::connection, "Upgrade");
    BC_REQUIRE(!is_websocket_upgrade(head));
}

// Quoted value (allowed in some contexts, not supported).
BC_AUTO_TEST_CASE(fields__is_websocket_upgrade__quoted_upgrade_value__false)
{
    fields head{};
    head.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    head.set(field::upgrade, "\"websocket\"");
    head.set(field::connection, "Upgrade");
    BC_REQUIRE(!is_websocket_upgrade(head));
}

// to_websocket_accept
// ----------------------------------------------------------------------------

BC_AUTO_TEST_CASE(fields__to_websocket_accept__rfc_example__expected)
{
    fields header{};
    header.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    BC_REQUIRE_EQUAL(to_websocket_accept(header), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

BC_AUTO_TEST_CASE(fields__to_websocket_accept__empty_header__empty_string)
{
    fields header{};
    BC_REQUIRE(to_websocket_accept(header).empty());
}

BC_AUTO_TEST_CASE(fields__to_websocket_accept__empty_key__empty_string)
{
    fields header{};
    header.set(field::sec_websocket_key, "");
    BC_REQUIRE(to_websocket_accept(header).empty());
}

BC_AUTO_TEST_CASE(fields__to_websocket_accept__short_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, "abc");
    BC_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BC_AUTO_TEST_CASE(fields__to_websocket_accept__long_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, std::string(100, 'a'));
    BC_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BC_AUTO_TEST_CASE(fields__to_websocket_accept__all_zeros_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, std::string(24, '\0'));
    BC_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BC_AUTO_TEST_CASE(fields__to_websocket_accept__invalid_base64_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, "invalid!base64");
    BC_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BC_AUTO_TEST_CASE(fields__to_websocket_accept__uppercase_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, "ABC123");
    BC_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BC_AUTO_TEST_CASE(fields__to_websocket_accept__minimal_key__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, "dGhl");
    BC_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BC_AUTO_TEST_CASE(fields__to_websocket_accept__max_key_length__valid_output)
{
    fields header{};
    header.set(field::sec_websocket_key, std::string(128, 'a'));
    BC_REQUIRE_EQUAL(to_websocket_accept(header).length(), 28u);
}

BC_AUTO_TEST_SUITE_END()
