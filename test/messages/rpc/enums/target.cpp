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
#include "../../../test.hpp"

using namespace network::messages::rpc;

BOOST_FIXTURE_TEST_SUITE(rpc_target_tests, test::directory_setup_fixture)

// sanitize_origin

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__empty__empty)
{
    // empty is an invalid origin
    const auto path = sanitize_origin("/home/name", "");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__no_leading_slash__empty)
{
    // leading non-slash is an invalid origin
    const auto path = sanitize_origin("/home/name", "path");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__leading_double_slash__empty)
{
    // leading double-slash is an invalid origin
    const auto path = sanitize_origin("/home/name", "//");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__leading_scheme__empty)
{
    // leading scheme is an invalid origin
    const auto path = sanitize_origin("/home/name", "http://");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__slash_only__expected)
{
    const auto path = sanitize_origin("/home/name", "/");
    BOOST_REQUIRE_EQUAL(path.string(), "/home/name/");
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__slashed_path__concatenated)
{
    const auto path = sanitize_origin("/home/name", "/path/foo/bar.ext");
    BOOST_REQUIRE_EQUAL(path.string(), "/home/name/path/foo/bar.ext");
}

// get_file_body

BOOST_AUTO_TEST_CASE(rpc_target__get_file_body__exists__is_open)
{
    BOOST_REQUIRE(test::create(TEST_PATH));

    const auto file = get_file_body(TEST_PATH);
    BOOST_REQUIRE(file.is_open());
}

BOOST_AUTO_TEST_CASE(rpc_target__get_file_body__not_exists__not_is_open)
{
    BOOST_REQUIRE(test::create(TEST_PATH));

    const auto file = get_file_body(TEST_PATH + "42");
    BOOST_REQUIRE(!file.is_open());
}

BOOST_AUTO_TEST_CASE(rpc_target__get_file_body__invalid_characters__does_not_throw)
{
    const auto file = get_file_body("~`!@#$%^&*()-+=,;:{}][");
    BOOST_REQUIRE(!file.is_open());
}

// get_mime_type

BOOST_AUTO_TEST_CASE(rpc_target__get_mime_type__not_found__default)
{
    const std::string default_get_mime_type{ "application/octet-stream" };
    BOOST_REQUIRE_EQUAL(get_mime_type(""), default_get_mime_type);
    BOOST_REQUIRE_EQUAL(get_mime_type("."), default_get_mime_type);
    BOOST_REQUIRE_EQUAL(get_mime_type(".42"), default_get_mime_type);
    BOOST_REQUIRE_EQUAL(get_mime_type(".xml."), default_get_mime_type);
}

BOOST_AUTO_TEST_CASE(rpc_target__get_mime_type__lower_case_exist__expected)
{
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.html"), "text/html");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.htm"), "text/html");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.css"), "text/css");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.js"), "application/javascript");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.json"), "application/json");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.xml"), "application/xml");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.txt"), "text/plain");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.png"), "image/png");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.jpg"), "image/jpeg");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.jpeg"), "image/jpeg");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.gif"), "image/gif");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.svg"), "image/svg+xml");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.ico"), "image/x-icon");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.pdf"), "application/pdf");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.zip"), "application/zip");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.mp4"), "video/mp4");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.mp3"), "audio/mpeg");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.woff"), "font/woff");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.woff2"), "font/woff2");
}

BOOST_AUTO_TEST_CASE(rpc_target__get_mime_type__mixed_case_exist__expected)
{
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.hTml"), "text/html");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.htM"), "text/html");
    BOOST_REQUIRE_EQUAL(get_mime_type("foo/bar.CSS"), "text/css");
}

BOOST_AUTO_TEST_SUITE_END()
