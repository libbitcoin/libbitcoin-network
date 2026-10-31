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

BOOST_AUTO_TEST_SUITE(ws_handshake_tests)

using namespace network::http;

static request_header make_request()
{
    request_header request{ verb::get, "/", version_1_1 };
    request.set(field::host, "example.com");
    request.set(field::connection, "Upgrade");
    request.set(field::upgrade, "websocket");
    request.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ==");
    request.set(field::sec_websocket_version, "13");
    return request;
}

BOOST_AUTO_TEST_CASE(handshake__accept__rfc_request__switching_protocols)
{
    ws::response response{};
    BOOST_REQUIRE_EQUAL(ws::accept(response, make_request()), error::success);
    BOOST_REQUIRE(response.result() == status::switching_protocols);
    BOOST_REQUIRE_EQUAL(response[field::upgrade], "websocket");
    BOOST_REQUIRE_EQUAL(response[field::connection], "Upgrade");
    BOOST_REQUIRE_EQUAL(response[field::sec_websocket_accept], "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
}

BOOST_AUTO_TEST_CASE(handshake__accept__token_lists__switching_protocols)
{
    auto request = make_request();
    request.set(field::connection, "keep-alive, UPGRADE");
    request.set(field::upgrade, "h2c, WebSocket");
    ws::response response{};
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::success);
    BOOST_REQUIRE(response.result() == status::switching_protocols);
}

BOOST_AUTO_TEST_CASE(handshake__accept__http_1_0__bad_http_version)
{
    auto request = make_request();
    request.version(version_1_0);
    ws::response response{};
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::bad_http_version);
    BOOST_REQUIRE(response.result() == status::bad_request);
    BOOST_REQUIRE_EQUAL(response.body(), code{ error::bad_http_version }.message());
}

BOOST_AUTO_TEST_CASE(handshake__accept__post__websocket_bad_method)
{
    auto request = make_request();
    request.method(verb::post);
    ws::response response{};
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::websocket_bad_method);
    BOOST_REQUIRE(response.result() == status::bad_request);
}

BOOST_AUTO_TEST_CASE(handshake__accept__missing_fields__expected_errors)
{
    ws::response response{};
    auto request = make_request();
    request.erase(field::host);
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::no_host);

    request = make_request();
    request.erase(field::connection);
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::no_connection);

    request = make_request();
    request.erase(field::upgrade);
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::no_upgrade);

    request = make_request();
    request.erase(field::sec_websocket_key);
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::no_sec_key);

    request = make_request();
    request.erase(field::sec_websocket_version);
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::no_sec_version);
    BOOST_REQUIRE(response.result() == status::bad_request);
}

BOOST_AUTO_TEST_CASE(handshake__accept__wrong_tokens__expected_errors)
{
    ws::response response{};
    auto request = make_request();
    request.set(field::connection, "keep-alive");
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::no_connection_upgrade);

    request = make_request();
    request.set(field::upgrade, "h2c");
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::no_upgrade_websocket);

    request = make_request();
    request.set(field::sec_websocket_key, "dGhlIHNhbXBsZSBub25jZQ===");
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::bad_sec_key);
}

BOOST_AUTO_TEST_CASE(handshake__accept__version_8__upgrade_required)
{
    auto request = make_request();
    request.set(field::sec_websocket_version, "8");
    ws::response response{};
    BOOST_REQUIRE_EQUAL(ws::accept(response, request), error::bad_sec_version);
    BOOST_REQUIRE(response.result() == status::upgrade_required);
    BOOST_REQUIRE_EQUAL(response[field::sec_websocket_version], "13");
}

BOOST_AUTO_TEST_SUITE_END()
