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

using namespace system;
using namespace network::http;

BOOST_AUTO_TEST_SUITE(mime_type_tests)

// to_mime_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(mime_type__to_mime_type__special_characters__does_not_throw)
{
    BOOST_REQUIRE_NO_THROW(to_mime_type("~`!@#$%^&*()-+=,;:{}]["));
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_type__invalid__unknown)
{
    BOOST_REQUIRE(to_mime_type("") == mime_type::unknown);
    BOOST_REQUIRE(to_mime_type("invalid/type") == mime_type::unknown);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_type__invalid_with_default__default)
{
    BOOST_REQUIRE(to_mime_type("", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(to_mime_type("invalid/type", mime_type::font_woff2) == mime_type::font_woff2);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_type__valid__expected)
{
    BOOST_REQUIRE(to_mime_type("image/png") == mime_type::image_png);
    BOOST_REQUIRE(to_mime_type("text/html") == mime_type::text_html);
    BOOST_REQUIRE(to_mime_type("text/plain") == mime_type::text_plain);
    BOOST_REQUIRE(to_mime_type("application/json") == mime_type::application_json);
    BOOST_REQUIRE(to_mime_type("application/octet-stream") == mime_type::application_octet);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_type__case_insensitive__expected)
{
    BOOST_REQUIRE(to_mime_type("TEXT/HTML") == mime_type::text_html);
    BOOST_REQUIRE(to_mime_type("text/PLAIN") == mime_type::text_plain);
    BOOST_REQUIRE(to_mime_type("Application/Json") == mime_type::application_json);
}

// from_mime_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(mime_type__from_mime_type__valid__does_not_throw)
{
    BOOST_REQUIRE_NO_THROW(from_mime_type(mime_type::text_html));
    BOOST_REQUIRE_NO_THROW(from_mime_type(mime_type::application_octet));
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_type__invalid__unknown)
{
    BOOST_REQUIRE_EQUAL(from_mime_type(static_cast<mime_type>(999)), "unknown");
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_type__unknown__unknown)
{
    BOOST_REQUIRE_EQUAL(from_mime_type(mime_type::unknown), "unknown");
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_type__unknown_with_default__default)
{
    BOOST_REQUIRE_EQUAL(from_mime_type(mime_type::unknown, "DEFAULT"), "DEFAULT");
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_type__valid__expected)
{
    BOOST_REQUIRE_EQUAL(from_mime_type(mime_type::text_html), "text/html");
    BOOST_REQUIRE_EQUAL(from_mime_type(mime_type::text_plain), "text/plain");
    BOOST_REQUIRE_EQUAL(from_mime_type(mime_type::application_json), "application/json");
    BOOST_REQUIRE_EQUAL(from_mime_type(mime_type::application_octet), "application/octet-stream");
}

// to_mime_types
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(mime_type__to_mime_types__special_characters__does_not_throw)
{
    BOOST_REQUIRE_NO_THROW(to_mime_types("text/html; charset=\"UTF-8,example\",~`!@#$%^&*()"));
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_types__invalid__unknown)
{
    const mime_types expected{ mime_type::unknown };
    BOOST_REQUIRE(to_mime_types("image/foo,invalid/type") == expected);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_types__invalid_with_default__default)
{
    const mime_types expected{ mime_type::font_woff };
    BOOST_REQUIRE(to_mime_types("image/foo,invalid/type", mime_type::font_woff) == expected);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_types__empty__unknown)
{
    const mime_types expected{ mime_type::unknown };
    BOOST_REQUIRE(to_mime_types("") == expected);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_types__valid_and_special_characters__expected_unknown)
{
    const mime_types expected
    {
        mime_type::text_html,
        mime_type::unknown
    };

    BOOST_REQUIRE(to_mime_types("text/html; charset=\"UTF-8,example\",~`!@#$%^&*(),What's this?") == expected);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_types__valids__expected)
{
    const mime_types expected
    {
        mime_type::application_json,
        mime_type::text_html,
        mime_type::text_plain
    };

    BOOST_REQUIRE(to_mime_types("text/html,application/json,text/plain") == expected);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_types__duplicated_unsorted__expected_deduplicated_sorted)
{
    const mime_types expected
    {
        mime_type::application_json,
        mime_type::text_html,
        mime_type::text_plain
    };

    BOOST_REQUIRE(to_mime_types("text/html,text/plain,text/html,application/json,text/plain") == expected);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_types__case_insensitive__expected)
{
    const mime_types expected
    {
        mime_type::application_json,
        mime_type::text_html,
        mime_type::text_plain
    };

    BOOST_REQUIRE(to_mime_types("TEXT/HTML,Application/Json,text/PLAIN") == expected);
}

BOOST_AUTO_TEST_CASE(mime_type__to_mime_types__with_parameters__ignores_parameters_expected)
{
    const mime_types expected
    {
        mime_type::application_json,
        mime_type::text_html
    };

    BOOST_REQUIRE(to_mime_types("text/html; charset=UTF-8, application/json;q=0.9") == expected);
}

// from_mime_types
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(mime_type__from_mime_types__invalid__does_not_throw)
{
    BOOST_REQUIRE_NO_THROW(from_mime_types({ static_cast<mime_type>(999) }));
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_types__empty__empty)
{
    BOOST_REQUIRE(from_mime_types({}).empty());
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_types__unknown__unknown)
{
    const mime_types types{ mime_type::unknown };
    BOOST_REQUIRE_EQUAL(from_mime_types(types), "unknown");
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_types__unknown_default__default)
{
    const mime_types types{ mime_type::unknown };
    BOOST_REQUIRE_EQUAL(from_mime_types(types, "DEFAULT"), "DEFAULT");
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_types__valid__expected)
{
    const mime_types types
    {
        mime_type::text_html,
        mime_type::text_plain,
        mime_type::application_json
    };

    const auto tokens = split(from_mime_types(types), ",");
    const auto expected = split("application/json,text/html,text/plain", ",");
    BOOST_REQUIRE_EQUAL(tokens, expected);
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_types__duplicated__expected_deduplicated)
{
    const mime_types types
    {
        mime_type::application_json,
        mime_type::text_html,
        mime_type::text_html
    };

    BOOST_REQUIRE_EQUAL(from_mime_types(types), "application/json,text/html");
}

BOOST_AUTO_TEST_CASE(mime_type__from_mime_types__unsotered__expected_sorted)
{
    const mime_types types
    {
        mime_type::text_html,
        mime_type::application_json
    };

    BOOST_REQUIRE_EQUAL(from_mime_types(types), "application/json,text/html");
}

// extension_mime_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(mime_type__extension_mime_type__not_found__default)
{
    BOOST_REQUIRE(extension_mime_type("") == mime_type::unknown);
    BOOST_REQUIRE(extension_mime_type(".") == mime_type::unknown);
    BOOST_REQUIRE(extension_mime_type(".42") == mime_type::unknown);
    BOOST_REQUIRE(extension_mime_type(".xml.") == mime_type::unknown);
    BOOST_REQUIRE(extension_mime_type("test123/test456/") == mime_type::unknown);
    BOOST_REQUIRE(extension_mime_type("test123/test456/file") == mime_type::unknown);
}

BOOST_AUTO_TEST_CASE(mime_type__extension_mime_type__not_found_default__default)
{
    BOOST_REQUIRE(extension_mime_type("", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(extension_mime_type(".", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(extension_mime_type(".42", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(extension_mime_type(".xml.", mime_type::font_woff) == mime_type::font_woff);
}

BOOST_AUTO_TEST_CASE(mime_type__extension_mime_type__lower_case_exist__expected)
{
    BOOST_REQUIRE(extension_mime_type(".html") == mime_type::text_html);
    BOOST_REQUIRE(extension_mime_type(".htm") == mime_type::text_html);
    BOOST_REQUIRE(extension_mime_type(".css") == mime_type::text_css);
    BOOST_REQUIRE(extension_mime_type(".js") == mime_type::application_javascript);
    BOOST_REQUIRE(extension_mime_type(".json") == mime_type::application_json);
    BOOST_REQUIRE(extension_mime_type(".xml") == mime_type::application_xml);
    BOOST_REQUIRE(extension_mime_type(".txt") == mime_type::text_plain);
    BOOST_REQUIRE(extension_mime_type(".png") == mime_type::image_png);
    BOOST_REQUIRE(extension_mime_type(".jpg") == mime_type::image_jpeg);
    BOOST_REQUIRE(extension_mime_type(".jpeg") == mime_type::image_jpeg);
    BOOST_REQUIRE(extension_mime_type(".gif") == mime_type::image_gif);
    BOOST_REQUIRE(extension_mime_type(".svg") == mime_type::image_svg_xml);
    BOOST_REQUIRE(extension_mime_type(".ico") == mime_type::image_x_icon);
    BOOST_REQUIRE(extension_mime_type(".pdf") == mime_type::application_pdf);
    BOOST_REQUIRE(extension_mime_type(".zip") == mime_type::application_zip);
    BOOST_REQUIRE(extension_mime_type(".mp4") == mime_type::video_mp4);
    BOOST_REQUIRE(extension_mime_type(".mp3") == mime_type::audio_mpeg);
    BOOST_REQUIRE(extension_mime_type(".woff") == mime_type::font_woff);
    BOOST_REQUIRE(extension_mime_type(".woff2") == mime_type::font_woff2);
}

BOOST_AUTO_TEST_CASE(mime_type__extension_mime_type__mixed_case_exist__expected)
{
    BOOST_REQUIRE(extension_mime_type(".hTml") == mime_type::text_html);
    BOOST_REQUIRE(extension_mime_type(".htM") == mime_type::text_html);
    BOOST_REQUIRE(extension_mime_type(".CSS") == mime_type::text_css);
}

// file_mime_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(mime_type__file_mime_type__not_found__default)
{
    BOOST_REQUIRE(file_mime_type("") == mime_type::unknown);
    BOOST_REQUIRE(file_mime_type(".") == mime_type::unknown);
    BOOST_REQUIRE(file_mime_type(".42") == mime_type::unknown);
    BOOST_REQUIRE(file_mime_type(".xml.") == mime_type::unknown);
}

BOOST_AUTO_TEST_CASE(mime_type__file_mime_type__not_found_default__default)
{
    BOOST_REQUIRE(file_mime_type("", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(file_mime_type(".", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(file_mime_type(".42", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(file_mime_type(".xml.", mime_type::font_woff) == mime_type::font_woff);
}

BOOST_AUTO_TEST_CASE(mime_type__file_mime_type__lower_case_exist__expected)
{
    BOOST_REQUIRE(file_mime_type("foo/bar.html") == mime_type::text_html);
    BOOST_REQUIRE(file_mime_type("foo/bar.htm") == mime_type::text_html);
    BOOST_REQUIRE(file_mime_type("foo/bar.css") == mime_type::text_css);
    BOOST_REQUIRE(file_mime_type("foo/bar.js") == mime_type::application_javascript);
    BOOST_REQUIRE(file_mime_type("foo/bar.json") == mime_type::application_json);
    BOOST_REQUIRE(file_mime_type("foo/bar.xml") == mime_type::application_xml);
    BOOST_REQUIRE(file_mime_type("foo/bar.txt") == mime_type::text_plain);
    BOOST_REQUIRE(file_mime_type("foo/bar.png") == mime_type::image_png);
    BOOST_REQUIRE(file_mime_type("foo/bar.jpg") == mime_type::image_jpeg);
    BOOST_REQUIRE(file_mime_type("foo/bar.jpeg") == mime_type::image_jpeg);
    BOOST_REQUIRE(file_mime_type("foo/bar.gif") == mime_type::image_gif);
    BOOST_REQUIRE(file_mime_type("foo/bar.svg") == mime_type::image_svg_xml);
    BOOST_REQUIRE(file_mime_type("foo/bar.ico") == mime_type::image_x_icon);
    BOOST_REQUIRE(file_mime_type("foo/bar.pdf") == mime_type::application_pdf);
    BOOST_REQUIRE(file_mime_type("foo/bar.zip") == mime_type::application_zip);
    BOOST_REQUIRE(file_mime_type("foo/bar.mp4") == mime_type::video_mp4);
    BOOST_REQUIRE(file_mime_type("foo/bar.mp3") == mime_type::audio_mpeg);
    BOOST_REQUIRE(file_mime_type("foo/bar.woff") == mime_type::font_woff);
    BOOST_REQUIRE(file_mime_type("foo/bar.woff2") == mime_type::font_woff2);
}

BOOST_AUTO_TEST_CASE(mime_type__file_mime_type__mixed_case_exist__expected)
{
    BOOST_REQUIRE(file_mime_type("foo/bar.hTml") == mime_type::text_html);
    BOOST_REQUIRE(file_mime_type("foo/bar.htM") == mime_type::text_html);
    BOOST_REQUIRE(file_mime_type("foo/bar.CSS") == mime_type::text_css);
}

// content_mime_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(mime_type__content_mime_type__not_found__default)
{
    BOOST_REQUIRE(content_mime_type("") == mime_type::unknown);
    BOOST_REQUIRE(content_mime_type("invalid/type") == mime_type::unknown);
    BOOST_REQUIRE(content_mime_type("text/invalid") == mime_type::unknown);
    BOOST_REQUIRE(content_mime_type(";charset=utf-8") == mime_type::unknown);
}

BOOST_AUTO_TEST_CASE(mime_type__content_mime_type__not_found_default__default)
{
    BOOST_REQUIRE(content_mime_type("", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(content_mime_type("invalid/type", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(content_mime_type("text/invalid", mime_type::font_woff) == mime_type::font_woff);
    BOOST_REQUIRE(content_mime_type(";charset=utf-8", mime_type::font_woff) == mime_type::font_woff);
}

BOOST_AUTO_TEST_CASE(mime_type__content_mime_type__lower_case_exist__expected)
{
    BOOST_REQUIRE(content_mime_type("application/javascript") == mime_type::application_javascript);
    BOOST_REQUIRE(content_mime_type("application/json") == mime_type::application_json);
    BOOST_REQUIRE(content_mime_type("application/octet-stream") == mime_type::application_octet);
    BOOST_REQUIRE(content_mime_type("application/pdf") == mime_type::application_pdf);
    BOOST_REQUIRE(content_mime_type("application/xml") == mime_type::application_xml);
    BOOST_REQUIRE(content_mime_type("application/zip") == mime_type::application_zip);
    BOOST_REQUIRE(content_mime_type("audio/mpeg") == mime_type::audio_mpeg);
    BOOST_REQUIRE(content_mime_type("font/woff") == mime_type::font_woff);
    BOOST_REQUIRE(content_mime_type("font/woff2") == mime_type::font_woff2);
    BOOST_REQUIRE(content_mime_type("image/gif") == mime_type::image_gif);
    BOOST_REQUIRE(content_mime_type("image/jpeg") == mime_type::image_jpeg);
    BOOST_REQUIRE(content_mime_type("image/png") == mime_type::image_png);
    BOOST_REQUIRE(content_mime_type("image/svg+xml") == mime_type::image_svg_xml);
    BOOST_REQUIRE(content_mime_type("image/x-icon") == mime_type::image_x_icon);
    BOOST_REQUIRE(content_mime_type("text/css") == mime_type::text_css);
    BOOST_REQUIRE(content_mime_type("text/html") == mime_type::text_html);
    BOOST_REQUIRE(content_mime_type("text/plain") == mime_type::text_plain);
    BOOST_REQUIRE(content_mime_type("video/mp4") == mime_type::video_mp4);
}

BOOST_AUTO_TEST_CASE(mime_type__content_mime_type__mixed_case_exist__expected)
{
    BOOST_REQUIRE(content_mime_type("APPLICATION/JAVASCRIPT") == mime_type::application_javascript);
    BOOST_REQUIRE(content_mime_type("application/JSON") == mime_type::application_json);
    BOOST_REQUIRE(content_mime_type("TEXT/PLAIN") == mime_type::text_plain);
}

BOOST_AUTO_TEST_CASE(mime_type__content_mime_type__with_parameters__expected)
{
    BOOST_REQUIRE(content_mime_type("application/json; charset=utf-8") == mime_type::application_json);
    BOOST_REQUIRE(content_mime_type("text/plain; charset=iso-8859-1") == mime_type::text_plain);
    BOOST_REQUIRE(content_mime_type("application/octet-stream; boundary=abc") == mime_type::application_octet);
    BOOST_REQUIRE(content_mime_type("text/html; charset=utf-8; other=param") == mime_type::text_html);
}

BOOST_AUTO_TEST_SUITE_END()
