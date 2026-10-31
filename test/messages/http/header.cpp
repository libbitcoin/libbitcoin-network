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

BC_AUTO_TEST_SUITE(header_tests)

using namespace network::http;

// enums

BC_AUTO_TEST_CASE(header__to_verb__tokens__expected)
{
    BC_REQUIRE(to_verb("GET") == verb::get);
    BC_REQUIRE(to_verb("DELETE") == verb::delete_);
    BC_REQUIRE(to_verb("M-SEARCH") == verb::msearch);
    BC_REQUIRE(to_verb("get") == verb::unknown);
    BC_REQUIRE(to_verb("FOO") == verb::unknown);
    BC_REQUIRE_EQUAL(to_string(verb::post), "POST");
    BC_REQUIRE(to_string(verb::unknown).empty());
    BC_REQUIRE(to_string(verb::websocket).empty());
}

BC_AUTO_TEST_CASE(header__to_status__codes__expected)
{
    BC_REQUIRE(to_status(200) == status::ok);
    BC_REQUIRE(to_status(599) == status::network_connect_timeout_error);
    BC_REQUIRE(to_status(299) == status::unknown);
    BC_REQUIRE_EQUAL(status_string(status::not_found), "Not Found");
    BC_REQUIRE_EQUAL(status_string(status::unknown), "<unknown-status>");
}

BC_AUTO_TEST_CASE(header__to_field__names__case_insensitive)
{
    BC_REQUIRE(to_field("content-length") == field::content_length);
    BC_REQUIRE(to_field("CONTENT-TYPE") == field::content_type);
    BC_REQUIRE(to_field("X-Bogus") == field::unknown);
    BC_REQUIRE_EQUAL(to_string(field::sec_websocket_key), "Sec-WebSocket-Key");
}

// fields

BC_AUTO_TEST_CASE(header__fields__insert_set_erase__expected)
{
    fields instance{};
    instance.insert(field::accept, "a");
    instance.insert("X-Other", "x");
    instance.insert("accept", "b");
    BC_REQUIRE_EQUAL(instance.size(), 3u);
    BC_REQUIRE_EQUAL(instance.count(field::accept), 2u);
    BC_REQUIRE_EQUAL(instance[field::accept], "a");
    BC_REQUIRE_EQUAL(instance.begin()->name, "Accept");
    BC_REQUIRE_EQUAL(std::next(instance.begin())->value, "b");

    instance.set("ACCEPT", "c");
    BC_REQUIRE_EQUAL(instance.count(field::accept), 1u);
    BC_REQUIRE_EQUAL(instance[field::accept], "c");
    BC_REQUIRE_EQUAL(instance.erase("x-other"), 1u);
    BC_REQUIRE(instance.find("X-Other") == instance.end());
    BC_REQUIRE(instance["X-Other"].empty());
}

BC_AUTO_TEST_CASE(header__has_token__lists__expected)
{
    BC_REQUIRE(has_token("keep-alive, Upgrade", "upgrade"));
    BC_REQUIRE(has_token(" close ", "close"));
    BC_REQUIRE(!has_token("closed", "close"));
    BC_REQUIRE(!has_token("", "close"));
}

// header_base

BC_AUTO_TEST_CASE(header__keep_alive__http_1_1__default_true)
{
    request_header instance{ verb::get, "/", version_1_1 };
    BC_REQUIRE(instance.keep_alive());

    instance.set(field::connection, "Close");
    BC_REQUIRE(!instance.keep_alive());

    instance.keep_alive(true);
    BC_REQUIRE(instance.keep_alive());
    BC_REQUIRE(instance.find(field::connection) == instance.end());

    instance.keep_alive(false);
    BC_REQUIRE_EQUAL(instance[field::connection], "close");
}

BC_AUTO_TEST_CASE(header__keep_alive__http_1_0__default_false)
{
    request_header instance{ verb::get, "/", version_1_0 };
    BC_REQUIRE(!instance.keep_alive());

    instance.keep_alive(true);
    BC_REQUIRE_EQUAL(instance[field::connection], "keep-alive");
    BC_REQUIRE(instance.keep_alive());

    instance.set(field::connection, "upgrade, keep-alive");
    instance.keep_alive(false);
    BC_REQUIRE_EQUAL(instance[field::connection], "upgrade");
}

BC_AUTO_TEST_CASE(header__chunked__set_clear__expected)
{
    response_header instance{ status::ok, version_1_1 };
    instance.content_length(42u);
    BC_REQUIRE_EQUAL(instance.content_length().value(), 42u);

    instance.chunked(true);
    BC_REQUIRE(instance.chunked());
    BC_REQUIRE(!instance.content_length().has_value());
    BC_REQUIRE_EQUAL(instance[field::transfer_encoding], "chunked");

    instance.set(field::transfer_encoding, "gzip, chunked");
    instance.chunked(false);
    BC_REQUIRE(!instance.chunked());
    BC_REQUIRE_EQUAL(instance[field::transfer_encoding], "gzip");
}

BC_AUTO_TEST_CASE(header__method_string__unknown__retained)
{
    request_header instance{};
    instance.method_string("FOO");
    BC_REQUIRE(instance.method() == verb::unknown);
    BC_REQUIRE_EQUAL(instance.method_string(), "FOO");

    instance.method_string("PUT");
    BC_REQUIRE(instance.method() == verb::put);
    BC_REQUIRE_EQUAL(instance.method_string(), "PUT");
}

BC_AUTO_TEST_CASE(header__reason__default__standard_phrase)
{
    response_header instance{ status::not_found, version_1_1 };
    BC_REQUIRE_EQUAL(instance.reason(), "Not Found");
    BC_REQUIRE_EQUAL(instance.result_int(), 404u);

    instance.reason("Gone Fishing");
    BC_REQUIRE_EQUAL(instance.reason(), "Gone Fishing");
}

BC_AUTO_TEST_CASE(header__need_eof__response__expected)
{
    response_header instance{ status::ok, version_1_1 };
    BC_REQUIRE(instance.need_eof());

    instance.chunked(true);
    BC_REQUIRE(!instance.need_eof());

    instance.keep_alive(false);
    BC_REQUIRE(instance.need_eof());
}

BC_AUTO_TEST_CASE(header__is_upgrade__websocket__expected)
{
    request_header instance{ verb::get, "/", version_1_1 };
    instance.set(field::connection, "keep-alive, Upgrade");
    instance.set(field::upgrade, "websocket");
    BC_REQUIRE(is_upgrade(instance));

    instance.version(version_1_0);
    BC_REQUIRE(!is_upgrade(instance));
}

// message

BC_AUTO_TEST_CASE(header__prepare_payload__versions__expected)
{
    message<false, string_body> instance{ status::ok, version_1_1 };
    instance.content_length(5u);
    instance.prepare_payload();
    BC_REQUIRE(instance.chunked());
    BC_REQUIRE(!instance.content_length().has_value());

    instance.version(version_1_0);
    instance.prepare_payload();
    BC_REQUIRE(!instance.chunked());

    instance.result(status::no_content);
    instance.content_length(5u);
    instance.prepare_payload();
    BC_REQUIRE(!instance.content_length().has_value());
    BC_REQUIRE(!instance.chunked());
}

// flat_buffer

BC_AUTO_TEST_CASE(header__flat_buffer__prepare_commit_consume__expected)
{
    flat_buffer instance{ 8 };
    auto out = instance.prepare(5);
    std::memcpy(out.data(), "hello", 5);
    instance.commit(5);
    BC_REQUIRE_EQUAL(instance.size(), 5u);

    instance.consume(2);
    BC_REQUIRE_EQUAL(std::string(static_cast<const char*>(instance.data().data()), instance.size()), "llo");

    out = instance.prepare(5);
    std::memcpy(out.data(), "world", 5);
    instance.commit(5);
    BC_REQUIRE_EQUAL(std::string(static_cast<const char*>(instance.data().data()), instance.size()), "lloworld");
    BC_REQUIRE_THROW(instance.prepare(1), std::length_error);

    instance.consume(100);
    BC_REQUIRE_EQUAL(instance.size(), 0u);
}

BC_AUTO_TEST_SUITE_END()
