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

#if defined (HAVE_MSC)
    static const std::string root{ "c:\\home\\name" };
    static const std::string norm{ "c:/home/name" };
#else
    static const std::string root{ "/home/name" };
    static const std::string norm{ "/home/name" };
#endif

// is_origin_form
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_target__is_origin_form__path_with_query__true)
{
    BOOST_REQUIRE(is_origin_form("/index.html?field=value"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_origin_form__malformed_uri__false)
{
    BOOST_REQUIRE(!is_origin_form("/[invalid"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_origin_form__oversized__false)
{
    const std::string oversized(max_url, 'a');
    BOOST_REQUIRE(!is_origin_form("/" + oversized));
}

// is_absolute_form
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_target__is_absolute_form__http_uri__true)
{
    BOOST_REQUIRE(is_absolute_form("http://www.boost.org/index.html"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_absolute_form__ftp_scheme__false)
{
    BOOST_REQUIRE(!is_absolute_form("ftp://example.com"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_absolute_form__malformed_uri__false)
{
    BOOST_REQUIRE(!is_absolute_form("http://[invalid"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_absolute_form__oversized__false)
{
    const std::string oversized(max_url, 'a');
    BOOST_REQUIRE(!is_absolute_form("/" + oversized));
}

// is_authority_form
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_target__is_authority_form__valid_authority__true)
{
    BOOST_REQUIRE(is_authority_form("//user@host:8080"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_authority_form__no_leading_slashes__false)
{
    BOOST_REQUIRE(!is_authority_form("host:8080"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_authority_form__oversized__false)
{
    const std::string oversized(max_url, 'a');
    BOOST_REQUIRE(!is_authority_form("/" + oversized));
}

// is_asterisk_form
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_target__is_asterisk_form__asterisk__true)
{
    BOOST_REQUIRE(is_asterisk_form("*"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_asterisk_form__non_asterisk__false)
{
    BOOST_REQUIRE(!is_asterisk_form("/"));
}

// to_target
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_target__to_target__get_origin__origin)
{
    BOOST_REQUIRE(to_target("/index.html", http::verb::get) == target::origin);
}

BOOST_AUTO_TEST_CASE(rpc_target__to_target__options_asterisk__asterisk)
{
    BOOST_REQUIRE(to_target("*", http::verb::options) == target::asterisk);
}

BOOST_AUTO_TEST_CASE(rpc_target__to_target__connect_authority__authority)
{
    BOOST_REQUIRE(to_target("//host:8080", http::verb::connect) == target::authority);
}

BOOST_AUTO_TEST_CASE(rpc_target__to_target__unknown_method__unknown)
{
    BOOST_REQUIRE(to_target("/index.html", http::verb::unknown) == target::unknown);
}

BOOST_AUTO_TEST_CASE(rpc_target__to_target__invalid_target__unknown)
{
    BOOST_REQUIRE(to_target("http://[invalid", http::verb::get) == target::unknown);
}

// sanitize_origin
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__empty__empty)
{
    // empty is an invalid origin
    const auto path = sanitize_origin(root, "");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__no_leading_slash__empty)
{
    // leading non-slash is an invalid origin
    const auto path = sanitize_origin(root, "path");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__leading_double_slash__empty)
{
    // leading double-slash is an invalid origin
    const auto path = sanitize_origin(root, "//");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__leading_scheme__empty)
{
    // leading scheme is an invalid origin
    const auto path = sanitize_origin(root, "http://");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__slash_only__empty)
{
    // slash only target must be defaulted (e.g. /index.html) before sanitize.
    const auto path = sanitize_origin(root, "/");
    BOOST_REQUIRE(path.empty());
}

// is_safe_target
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__slashed_path__concatenated)
{
    const auto path = sanitize_origin(root, "/path/foo/bar.ext");
    BOOST_REQUIRE_EQUAL(path.generic_string(), norm + "/path/foo/bar.ext");
}

BOOST_AUTO_TEST_CASE(rpc_target__is_safe_target__abstract_socket__false)
{
    BOOST_REQUIRE(!is_safe_target("/@socket"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_safe_target__port_like__false)
{
    BOOST_REQUIRE(!is_safe_target("/path:foo"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_safe_target__path_traversal__false)
{
    BOOST_REQUIRE(!is_safe_target("/path/../bar"));
}

BOOST_AUTO_TEST_CASE(rpc_target__is_safe_target__single_segment__true)
{
    BOOST_REQUIRE(is_safe_target("/file.ext"));
}

// to_canonical
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(rpc_target__to_canonical__non_existent_root__concatenated)
{
    const auto path = to_canonical(root + "/nonexistent", "/file.ext");
    BOOST_REQUIRE_EQUAL(path.generic_string(), norm + "/nonexistent/file.ext");
}

BOOST_AUTO_TEST_CASE(rpc_target__to_canonical__non_absolute_root__empty)
{
    const auto path = to_canonical("relative/path", "/file.ext");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__to_canonical__escaping_path__empty)
{
    const auto path = to_canonical(root, "/path/../../etc/passwd");
    BOOST_REQUIRE(path.empty());
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__valid_deep_path__concatenated)
{
    const auto path = sanitize_origin(root, "/deep/path/to/file.ext");
    BOOST_REQUIRE_EQUAL(path.generic_string(), norm + "/deep/path/to/file.ext");
}

BOOST_AUTO_TEST_CASE(rpc_target__sanitize_origin__valid_non_existent_file__concatenated)
{
    const auto path = sanitize_origin(root, "/new/file.ext");
    BOOST_REQUIRE_EQUAL(path.generic_string(), norm + "/new/file.ext");
}

// get_file_body
// ----------------------------------------------------------------------------

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
// ----------------------------------------------------------------------------

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
