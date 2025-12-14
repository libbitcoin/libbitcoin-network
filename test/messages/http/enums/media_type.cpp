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

BOOST_AUTO_TEST_SUITE(media_type_tests)

// to_media_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(media_type__to_media_type__special_characters__does_not_throw)
{
    BOOST_REQUIRE_NO_THROW(to_media_type("~`!@#$%^&*()-+=,;:{}]["));
}

BOOST_AUTO_TEST_CASE(media_type__to_media_type__invalid__unknown)
{
    BOOST_REQUIRE(to_media_type("") == media_type::unknown);
    BOOST_REQUIRE(to_media_type("invalid/type") == media_type::unknown);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_type__invalid_with_default__default)
{
    BOOST_REQUIRE(to_media_type("", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(to_media_type("invalid/type", media_type::font_woff2) == media_type::font_woff2);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_type__valid__expected)
{
    BOOST_REQUIRE(to_media_type("image/png") == media_type::image_png);
    BOOST_REQUIRE(to_media_type("text/html") == media_type::text_html);
    BOOST_REQUIRE(to_media_type("text/plain") == media_type::text_plain);
    BOOST_REQUIRE(to_media_type("application/json") == media_type::application_json);
    BOOST_REQUIRE(to_media_type("application/octet-stream") == media_type::application_octet_stream);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_type__case_insensitive__expected)
{
    BOOST_REQUIRE(to_media_type("TEXT/HTML") == media_type::text_html);
    BOOST_REQUIRE(to_media_type("text/PLAIN") == media_type::text_plain);
    BOOST_REQUIRE(to_media_type("Application/Json") == media_type::application_json);
}

// from_media_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(media_type__from_media_type__valid__does_not_throw)
{
    BOOST_REQUIRE_NO_THROW(from_media_type(media_type::text_html));
    BOOST_REQUIRE_NO_THROW(from_media_type(media_type::application_octet_stream));
}

BOOST_AUTO_TEST_CASE(media_type__from_media_type__invalid__unknown)
{
    BOOST_REQUIRE_EQUAL(from_media_type(static_cast<media_type>(999)), "unknown");
}

BOOST_AUTO_TEST_CASE(media_type__from_media_type__unknown__unknown)
{
    BOOST_REQUIRE_EQUAL(from_media_type(media_type::unknown), "unknown");
}

BOOST_AUTO_TEST_CASE(media_type__from_media_type__unknown_with_default__default)
{
    BOOST_REQUIRE_EQUAL(from_media_type(media_type::unknown, "DEFAULT"), "DEFAULT");
}

BOOST_AUTO_TEST_CASE(media_type__from_media_type__valid__expected)
{
    BOOST_REQUIRE_EQUAL(from_media_type(media_type::text_html), "text/html");
    BOOST_REQUIRE_EQUAL(from_media_type(media_type::text_plain), "text/plain");
    BOOST_REQUIRE_EQUAL(from_media_type(media_type::application_json), "application/json");
    BOOST_REQUIRE_EQUAL(from_media_type(media_type::application_octet_stream), "application/octet-stream");
}

// to_media_types
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(media_type__to_media_types__special_characters__does_not_throw)
{
    BOOST_REQUIRE_NO_THROW(to_media_types("text/html; charset=\"UTF-8,example\",~`!@#$%^&*()"));
}

BOOST_AUTO_TEST_CASE(media_type__to_media_types__invalid__unknown)
{
    const media_types expected{ media_type::unknown };
    BOOST_REQUIRE(to_media_types("image/foo,invalid/type") == expected);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_types__invalid_with_default__default)
{
    const media_types expected{ media_type::font_woff };
    BOOST_REQUIRE(to_media_types("image/foo,invalid/type", media_type::font_woff) == expected);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_types__empty__unknown)
{
    const media_types expected{ media_type::unknown };
    BOOST_REQUIRE(to_media_types("") == expected);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_types__valid_and_special_characters__expected_unknown)
{
    const media_types expected
    {
        media_type::text_html,
        media_type::unknown
    };

    BOOST_REQUIRE(to_media_types("text/html; charset=\"UTF-8,example\",~`!@#$%^&*(),What's this?") == expected);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_types__valids__expected)
{
    const media_types expected
    {
        media_type::application_json,
        media_type::text_html,
        media_type::text_plain
    };

    BOOST_REQUIRE(to_media_types("text/html,application/json,text/plain") == expected);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_types__duplicated_unsorted__expected_deduplicated_sorted)
{
    const media_types expected
    {
        media_type::application_json,
        media_type::text_html,
        media_type::text_plain
    };

    BOOST_REQUIRE(to_media_types("text/html,text/plain,text/html,application/json,text/plain") == expected);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_types__case_insensitive__expected)
{
    const media_types expected
    {
        media_type::application_json,
        media_type::text_html,
        media_type::text_plain
    };

    BOOST_REQUIRE(to_media_types("TEXT/HTML,Application/Json,text/PLAIN") == expected);
}

BOOST_AUTO_TEST_CASE(media_type__to_media_types__with_parameters__ignores_parameters_expected)
{
    const media_types expected
    {
        media_type::application_json,
        media_type::text_html
    };

    BOOST_REQUIRE(to_media_types("text/html; charset=UTF-8, application/json;q=0.9") == expected);
}

// from_media_types
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(media_type__from_media_types__invalid__does_not_throw)
{
    BOOST_REQUIRE_NO_THROW(from_media_types({ static_cast<media_type>(999) }));
}

BOOST_AUTO_TEST_CASE(media_type__from_media_types__empty__empty)
{
    BOOST_REQUIRE(from_media_types({}).empty());
}

BOOST_AUTO_TEST_CASE(media_type__from_media_types__unknown__unknown)
{
    const media_types types{ media_type::unknown };
    BOOST_REQUIRE_EQUAL(from_media_types(types), "unknown");
}

BOOST_AUTO_TEST_CASE(media_type__from_media_types__unknown_default__default)
{
    const media_types types{ media_type::unknown };
    BOOST_REQUIRE_EQUAL(from_media_types(types, "DEFAULT"), "DEFAULT");
}

BOOST_AUTO_TEST_CASE(media_type__from_media_types__valid__expected)
{
    const media_types types
    {
        media_type::text_html,
        media_type::text_plain,
        media_type::application_json
    };

    const auto tokens = split(from_media_types(types), ",");
    const auto expected = split("application/json,text/html,text/plain", ",");
    BOOST_REQUIRE_EQUAL(tokens, expected);
}

BOOST_AUTO_TEST_CASE(media_type__from_media_types__duplicated__expected_deduplicated)
{
    const media_types types
    {
        media_type::application_json,
        media_type::text_html,
        media_type::text_html
    };

    BOOST_REQUIRE_EQUAL(from_media_types(types), "application/json,text/html");
}

BOOST_AUTO_TEST_CASE(media_type__from_media_types__unsotered__expected_sorted)
{
    const media_types types
    {
        media_type::text_html,
        media_type::application_json
    };

    BOOST_REQUIRE_EQUAL(from_media_types(types), "application/json,text/html");
}

// extension_media_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(media_type__extension_media_type__not_found__default)
{
    BOOST_REQUIRE(extension_media_type("") == media_type::unknown);
    BOOST_REQUIRE(extension_media_type(".") == media_type::unknown);
    BOOST_REQUIRE(extension_media_type(".42") == media_type::unknown);
    BOOST_REQUIRE(extension_media_type(".xml.") == media_type::unknown);
    BOOST_REQUIRE(extension_media_type("test123/test456/") == media_type::unknown);
    BOOST_REQUIRE(extension_media_type("test123/test456/file") == media_type::unknown);
}

BOOST_AUTO_TEST_CASE(media_type__extension_media_type__not_found_default__default)
{
    BOOST_REQUIRE(extension_media_type("", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(extension_media_type(".", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(extension_media_type(".42", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(extension_media_type(".xml.", media_type::font_woff) == media_type::font_woff);
}

BOOST_AUTO_TEST_CASE(media_type__extension_media_type__lower_case_exist__expected)
{
    BOOST_REQUIRE(extension_media_type(".html") == media_type::text_html);
    BOOST_REQUIRE(extension_media_type(".htm") == media_type::text_html);
    BOOST_REQUIRE(extension_media_type(".css") == media_type::text_css);
    BOOST_REQUIRE(extension_media_type(".js") == media_type::application_javascript);
    BOOST_REQUIRE(extension_media_type(".json") == media_type::application_json);
    BOOST_REQUIRE(extension_media_type(".xml") == media_type::application_xml);
    BOOST_REQUIRE(extension_media_type(".txt") == media_type::text_plain);
    BOOST_REQUIRE(extension_media_type(".png") == media_type::image_png);
    BOOST_REQUIRE(extension_media_type(".jpg") == media_type::image_jpeg);
    BOOST_REQUIRE(extension_media_type(".jpeg") == media_type::image_jpeg);
    BOOST_REQUIRE(extension_media_type(".gif") == media_type::image_gif);
    BOOST_REQUIRE(extension_media_type(".svg") == media_type::image_svg_xml);
    BOOST_REQUIRE(extension_media_type(".ico") == media_type::image_x_icon);
    BOOST_REQUIRE(extension_media_type(".pdf") == media_type::application_pdf);
    BOOST_REQUIRE(extension_media_type(".zip") == media_type::application_zip);
    BOOST_REQUIRE(extension_media_type(".mp4") == media_type::video_mp4);
    BOOST_REQUIRE(extension_media_type(".mp3") == media_type::audio_mpeg);
    BOOST_REQUIRE(extension_media_type(".woff") == media_type::font_woff);
    BOOST_REQUIRE(extension_media_type(".woff2") == media_type::font_woff2);
}

BOOST_AUTO_TEST_CASE(media_type__extension_media_type__mixed_case_exist__expected)
{
    BOOST_REQUIRE(extension_media_type(".hTml") == media_type::text_html);
    BOOST_REQUIRE(extension_media_type(".htM") == media_type::text_html);
    BOOST_REQUIRE(extension_media_type(".CSS") == media_type::text_css);
}

// file_media_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(media_type__file_media_type__not_found__default)
{
    BOOST_REQUIRE(file_media_type("") == media_type::unknown);
    BOOST_REQUIRE(file_media_type(".") == media_type::unknown);
    BOOST_REQUIRE(file_media_type(".42") == media_type::unknown);
    BOOST_REQUIRE(file_media_type(".xml.") == media_type::unknown);
}

BOOST_AUTO_TEST_CASE(media_type__file_media_type__not_found_default__default)
{
    BOOST_REQUIRE(file_media_type("", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(file_media_type(".", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(file_media_type(".42", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(file_media_type(".xml.", media_type::font_woff) == media_type::font_woff);
}

BOOST_AUTO_TEST_CASE(media_type__file_media_type__lower_case_exist__expected)
{
    BOOST_REQUIRE(file_media_type("foo/bar.html") == media_type::text_html);
    BOOST_REQUIRE(file_media_type("foo/bar.htm") == media_type::text_html);
    BOOST_REQUIRE(file_media_type("foo/bar.css") == media_type::text_css);
    BOOST_REQUIRE(file_media_type("foo/bar.js") == media_type::application_javascript);
    BOOST_REQUIRE(file_media_type("foo/bar.json") == media_type::application_json);
    BOOST_REQUIRE(file_media_type("foo/bar.xml") == media_type::application_xml);
    BOOST_REQUIRE(file_media_type("foo/bar.txt") == media_type::text_plain);
    BOOST_REQUIRE(file_media_type("foo/bar.png") == media_type::image_png);
    BOOST_REQUIRE(file_media_type("foo/bar.jpg") == media_type::image_jpeg);
    BOOST_REQUIRE(file_media_type("foo/bar.jpeg") == media_type::image_jpeg);
    BOOST_REQUIRE(file_media_type("foo/bar.gif") == media_type::image_gif);
    BOOST_REQUIRE(file_media_type("foo/bar.svg") == media_type::image_svg_xml);
    BOOST_REQUIRE(file_media_type("foo/bar.ico") == media_type::image_x_icon);
    BOOST_REQUIRE(file_media_type("foo/bar.pdf") == media_type::application_pdf);
    BOOST_REQUIRE(file_media_type("foo/bar.zip") == media_type::application_zip);
    BOOST_REQUIRE(file_media_type("foo/bar.mp4") == media_type::video_mp4);
    BOOST_REQUIRE(file_media_type("foo/bar.mp3") == media_type::audio_mpeg);
    BOOST_REQUIRE(file_media_type("foo/bar.woff") == media_type::font_woff);
    BOOST_REQUIRE(file_media_type("foo/bar.woff2") == media_type::font_woff2);
}

BOOST_AUTO_TEST_CASE(media_type__file_media_type__mixed_case_exist__expected)
{
    BOOST_REQUIRE(file_media_type("foo/bar.hTml") == media_type::text_html);
    BOOST_REQUIRE(file_media_type("foo/bar.htM") == media_type::text_html);
    BOOST_REQUIRE(file_media_type("foo/bar.CSS") == media_type::text_css);
}

// content_media_type
// ----------------------------------------------------------------------------

BOOST_AUTO_TEST_CASE(media_type__content_media_type__not_found__default)
{
    BOOST_REQUIRE(content_media_type("") == media_type::unknown);
    BOOST_REQUIRE(content_media_type("invalid/type") == media_type::unknown);
    BOOST_REQUIRE(content_media_type("text/invalid") == media_type::unknown);
    BOOST_REQUIRE(content_media_type(";charset=utf-8") == media_type::unknown);
}

BOOST_AUTO_TEST_CASE(media_type__content_media_type__not_found_default__default)
{
    BOOST_REQUIRE(content_media_type("", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(content_media_type("invalid/type", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(content_media_type("text/invalid", media_type::font_woff) == media_type::font_woff);
    BOOST_REQUIRE(content_media_type(";charset=utf-8", media_type::font_woff) == media_type::font_woff);
}

BOOST_AUTO_TEST_CASE(media_type__content_media_type__lower_case_exist__expected)
{
    BOOST_REQUIRE(content_media_type("application/javascript") == media_type::application_javascript);
    BOOST_REQUIRE(content_media_type("application/json") == media_type::application_json);
    BOOST_REQUIRE(content_media_type("application/octet-stream") == media_type::application_octet_stream);
    BOOST_REQUIRE(content_media_type("application/pdf") == media_type::application_pdf);
    BOOST_REQUIRE(content_media_type("application/xml") == media_type::application_xml);
    BOOST_REQUIRE(content_media_type("application/zip") == media_type::application_zip);
    BOOST_REQUIRE(content_media_type("audio/mpeg") == media_type::audio_mpeg);
    BOOST_REQUIRE(content_media_type("font/woff") == media_type::font_woff);
    BOOST_REQUIRE(content_media_type("font/woff2") == media_type::font_woff2);
    BOOST_REQUIRE(content_media_type("image/gif") == media_type::image_gif);
    BOOST_REQUIRE(content_media_type("image/jpeg") == media_type::image_jpeg);
    BOOST_REQUIRE(content_media_type("image/png") == media_type::image_png);
    BOOST_REQUIRE(content_media_type("image/svg+xml") == media_type::image_svg_xml);
    BOOST_REQUIRE(content_media_type("image/x-icon") == media_type::image_x_icon);
    BOOST_REQUIRE(content_media_type("text/css") == media_type::text_css);
    BOOST_REQUIRE(content_media_type("text/html") == media_type::text_html);
    BOOST_REQUIRE(content_media_type("text/plain") == media_type::text_plain);
    BOOST_REQUIRE(content_media_type("video/mp4") == media_type::video_mp4);
}

BOOST_AUTO_TEST_CASE(media_type__content_media_type__whitespace_exist__expected)
{
    BOOST_REQUIRE(content_media_type(" application/json ") == media_type::application_json);
}

BOOST_AUTO_TEST_CASE(media_type__content_media_type__mixed_case_exist__expected)
{
    BOOST_REQUIRE(content_media_type("APPLICATION/JAVASCRIPT") == media_type::application_javascript);
    BOOST_REQUIRE(content_media_type("application/JSON") == media_type::application_json);
    BOOST_REQUIRE(content_media_type("TEXT/PLAIN") == media_type::text_plain);
}

BOOST_AUTO_TEST_CASE(media_type__content_media_type__with_parameters__expected)
{
    BOOST_REQUIRE(content_media_type("application/json; charset=utf-8") == media_type::application_json);
    BOOST_REQUIRE(content_media_type("text/plain; charset=iso-8859-1") == media_type::text_plain);
    BOOST_REQUIRE(content_media_type("application/octet-stream; boundary=abc") == media_type::application_octet_stream);
    BOOST_REQUIRE(content_media_type("text/html; charset=utf-8; other=param") == media_type::text_html);
}

BOOST_AUTO_TEST_CASE(media_type__content_media_type__fields__expected)
{
    http::fields fields{};
    fields.set(http::field::accept, "text/plain");
    fields.set(http::field::content_type, "application/json; charset=utf-8");
    BOOST_REQUIRE(content_media_type(fields) == media_type::application_json);
}

BOOST_AUTO_TEST_CASE(media_type__content_media_type__fields_defaults__expected)
{
    http::fields fields{};
    fields.set(http::field::accept, "text/plain");
    BOOST_REQUIRE(content_media_type(fields) == media_type::unknown);
    BOOST_REQUIRE(content_media_type(fields, media_type::application_json) == media_type::application_json);
}

BOOST_AUTO_TEST_SUITE_END()
