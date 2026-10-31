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

BOOST_AUTO_TEST_SUITE(header_tests)

using namespace network::http;

// enums

BOOST_AUTO_TEST_CASE(header__to_verb__tokens__expected)
{
    BOOST_REQUIRE(to_verb("GET") == verb::get);
    BOOST_REQUIRE(to_verb("DELETE") == verb::delete_);
    BOOST_REQUIRE(to_verb("M-SEARCH") == verb::msearch);
    BOOST_REQUIRE(to_verb("get") == verb::unknown);
    BOOST_REQUIRE(to_verb("FOO") == verb::unknown);
    BOOST_REQUIRE_EQUAL(to_string(verb::post), "POST");
    BOOST_REQUIRE(to_string(verb::unknown).empty());
    BOOST_REQUIRE(to_string(verb::websocket).empty());
}

BOOST_AUTO_TEST_CASE(header__to_status__codes__expected)
{
    BOOST_REQUIRE(to_status(200) == status::ok);
    BOOST_REQUIRE(to_status(599) == status::network_connect_timeout_error);
    BOOST_REQUIRE(to_status(299) == status::unknown);
    BOOST_REQUIRE_EQUAL(status_string(status::not_found), "Not Found");
    BOOST_REQUIRE_EQUAL(status_string(status::unknown), "<unknown-status>");
}

BOOST_AUTO_TEST_CASE(header__to_field__names__case_insensitive)
{
    BOOST_REQUIRE(to_field("content-length") == field::content_length);
    BOOST_REQUIRE(to_field("CONTENT-TYPE") == field::content_type);
    BOOST_REQUIRE(to_field("X-Bogus") == field::unknown);
    BOOST_REQUIRE_EQUAL(to_string(field::sec_websocket_key), "Sec-WebSocket-Key");
}

// fields

BOOST_AUTO_TEST_CASE(header__fields__insert_set_erase__expected)
{
    fields instance{};
    instance.insert(field::accept, "a");
    instance.insert("X-Other", "x");
    instance.insert("accept", "b");
    BOOST_REQUIRE_EQUAL(instance.size(), 3u);
    BOOST_REQUIRE_EQUAL(instance.count(field::accept), 2u);
    BOOST_REQUIRE_EQUAL(instance[field::accept], "a");
    BOOST_REQUIRE_EQUAL(instance.begin()->name, "Accept");
    BOOST_REQUIRE_EQUAL(std::next(instance.begin())->value, "b");

    instance.set("ACCEPT", "c");
    BOOST_REQUIRE_EQUAL(instance.count(field::accept), 1u);
    BOOST_REQUIRE_EQUAL(instance[field::accept], "c");
    BOOST_REQUIRE_EQUAL(instance.erase("x-other"), 1u);
    BOOST_REQUIRE(instance.find("X-Other") == instance.end());
    BOOST_REQUIRE(instance["X-Other"].empty());
}

BOOST_AUTO_TEST_CASE(header__has_token__lists__expected)
{
    BOOST_REQUIRE(has_token("keep-alive, Upgrade", "upgrade"));
    BOOST_REQUIRE(has_token(" close ", "close"));
    BOOST_REQUIRE(!has_token("closed", "close"));
    BOOST_REQUIRE(!has_token("", "close"));
}

// header_base

BOOST_AUTO_TEST_CASE(header__keep_alive__http_1_1__default_true)
{
    request_header instance{ verb::get, "/", version_1_1 };
    BOOST_REQUIRE(instance.keep_alive());

    instance.set(field::connection, "Close");
    BOOST_REQUIRE(!instance.keep_alive());

    instance.keep_alive(true);
    BOOST_REQUIRE(instance.keep_alive());
    BOOST_REQUIRE(instance.find(field::connection) == instance.end());

    instance.keep_alive(false);
    BOOST_REQUIRE_EQUAL(instance[field::connection], "close");
}

BOOST_AUTO_TEST_CASE(header__keep_alive__http_1_0__default_false)
{
    request_header instance{ verb::get, "/", version_1_0 };
    BOOST_REQUIRE(!instance.keep_alive());

    instance.keep_alive(true);
    BOOST_REQUIRE_EQUAL(instance[field::connection], "keep-alive");
    BOOST_REQUIRE(instance.keep_alive());

    instance.set(field::connection, "upgrade, keep-alive");
    instance.keep_alive(false);
    BOOST_REQUIRE_EQUAL(instance[field::connection], "upgrade");
}

BOOST_AUTO_TEST_CASE(header__chunked__set_clear__expected)
{
    response_header instance{ status::ok, version_1_1 };
    instance.content_length(42u);
    BOOST_REQUIRE_EQUAL(instance.content_length().value(), 42u);

    instance.chunked(true);
    BOOST_REQUIRE(instance.chunked());
    BOOST_REQUIRE(!instance.content_length().has_value());
    BOOST_REQUIRE_EQUAL(instance[field::transfer_encoding], "chunked");

    instance.set(field::transfer_encoding, "gzip, chunked");
    instance.chunked(false);
    BOOST_REQUIRE(!instance.chunked());
    BOOST_REQUIRE_EQUAL(instance[field::transfer_encoding], "gzip");
}

BOOST_AUTO_TEST_CASE(header__method_string__unknown__retained)
{
    request_header instance{};
    instance.method_string("FOO");
    BOOST_REQUIRE(instance.method() == verb::unknown);
    BOOST_REQUIRE_EQUAL(instance.method_string(), "FOO");

    instance.method_string("PUT");
    BOOST_REQUIRE(instance.method() == verb::put);
    BOOST_REQUIRE_EQUAL(instance.method_string(), "PUT");
}

BOOST_AUTO_TEST_CASE(header__reason__default__standard_phrase)
{
    response_header instance{ status::not_found, version_1_1 };
    BOOST_REQUIRE_EQUAL(instance.reason(), "Not Found");
    BOOST_REQUIRE_EQUAL(instance.result_int(), 404u);

    instance.reason("Gone Fishing");
    BOOST_REQUIRE_EQUAL(instance.reason(), "Gone Fishing");
}

BOOST_AUTO_TEST_CASE(header__need_eof__response__expected)
{
    response_header instance{ status::ok, version_1_1 };
    BOOST_REQUIRE(instance.need_eof());

    instance.chunked(true);
    BOOST_REQUIRE(!instance.need_eof());

    instance.keep_alive(false);
    BOOST_REQUIRE(instance.need_eof());
}

BOOST_AUTO_TEST_CASE(header__is_upgrade__websocket__expected)
{
    request_header instance{ verb::get, "/", version_1_1 };
    instance.set(field::connection, "keep-alive, Upgrade");
    instance.set(field::upgrade, "websocket");
    BOOST_REQUIRE(is_upgrade(instance));

    instance.version(version_1_0);
    BOOST_REQUIRE(!is_upgrade(instance));
}

// message

BOOST_AUTO_TEST_CASE(header__prepare_payload__versions__expected)
{
    message<false, string_body> instance{ status::ok, version_1_1 };
    instance.content_length(5u);
    instance.prepare_payload();
    BOOST_REQUIRE(instance.chunked());
    BOOST_REQUIRE(!instance.content_length().has_value());

    instance.version(version_1_0);
    instance.prepare_payload();
    BOOST_REQUIRE(!instance.chunked());

    instance.result(status::no_content);
    instance.content_length(5u);
    instance.prepare_payload();
    BOOST_REQUIRE(!instance.content_length().has_value());
    BOOST_REQUIRE(!instance.chunked());
}

// flat_buffer

BOOST_AUTO_TEST_CASE(header__flat_buffer__prepare_commit_consume__expected)
{
    flat_buffer instance{ 8 };
    auto out = instance.prepare(5);
    std::memcpy(out.data(), "hello", 5);
    instance.commit(5);
    BOOST_REQUIRE_EQUAL(instance.size(), 5u);

    instance.consume(2);
    BOOST_REQUIRE_EQUAL(std::string(static_cast<const char*>(instance.data().data()), instance.size()), "llo");

    out = instance.prepare(5);
    std::memcpy(out.data(), "world", 5);
    instance.commit(5);
    BOOST_REQUIRE_EQUAL(std::string(static_cast<const char*>(instance.data().data()), instance.size()), "lloworld");
    BOOST_REQUIRE_THROW(instance.prepare(1), std::length_error);

    instance.consume(100);
    BOOST_REQUIRE_EQUAL(instance.size(), 0u);
}

BOOST_AUTO_TEST_SUITE_END()
