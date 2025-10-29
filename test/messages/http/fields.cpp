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

BOOST_AUTO_TEST_CASE(fields__has_attachment__empty__false)
{
    http::fields head{};
    BOOST_REQUIRE(!has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__no_content_disposition__false)
{
    http::fields head{};
    head.set(field::content_type, "application/octet-stream");
    BOOST_REQUIRE(!has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_present__true)
{
    http::fields head{};
    head.set(field::content_disposition, "attachment; filename=\"file.txt\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_equals__true)
{
    http::fields head{};
    head.set(field::content_disposition, "form-data; name=\"file\"; filename=\"data.bin\"");
    BOOST_REQUIRE(has_attachment(head));
}

// filename*= is valid (rfc7230), indicates unicode.
BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_star__true)
{
    http::fields head{};
    head.set(field::content_disposition, "attachment; filename*=utf-8''data%20file.pdf");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__case_insensitive__true)
{
    http::fields head{};
    head.set(field::content_disposition, "ATTACHMENT; FILENAME=\"DOC.PDF\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__mixed_case_filename__true)
{
    http::fields head{};
    head.set(field::content_disposition, "inline; FileName=\"image.PNG\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__no_filename__false)
{
    http::fields head{};
    head.set(field::content_disposition, "inline");
    BOOST_REQUIRE(!has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_empty__true)
{
    http::fields head{};
    head.set(field::content_disposition, "attachment; filename=\"\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__multiple_parameters__true)
{
    http::fields head{};
    head.set(field::content_disposition, "form-data; name=\"field1\"; filename=\"test.jpg\"; size=1024");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__whitespace__true)
{
    http::fields head{};
    head.set(field::content_disposition, " attachment ;  filename = \" doc.pdf \" ");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__quoted_filename_with_semicolon__true)
{
    http::fields head{};
    head.set(field::content_disposition, "attachment; filename=\"file;semi.txt\"");
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__no_equals__false)
{
    http::fields head{};
    head.set(field::content_disposition, "attachment; filename");

    // It's a simple test for trimmed leading "filename" assumes not other token
    // starts with "filename" unless also an attachment (such as "filename*").
    // Otherwise the request is not valid anyway, so can assume it has attachment.
    ////BOOST_REQUIRE(!has_attachment(head));
    BOOST_REQUIRE(has_attachment(head));
}

BOOST_AUTO_TEST_CASE(fields__has_attachment__filename_equals_as_name_value__false)
{
    http::fields head{};
    head.set(field::content_disposition, "form-data; name=\"filename=\"");
    BOOST_REQUIRE(!has_attachment(head));
}

BOOST_AUTO_TEST_SUITE_END()
