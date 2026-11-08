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

BC_AUTO_TEST_SUITE(http_parser_tests)

using namespace network::http;
using namespace network::error;

template <bool IsRequest>
using string_parser = parser<IsRequest, string_body>;

// Feed the text in one put per size step, returning the code and setting the
// unconsumed remainder.
template <bool IsRequest>
static code feed(string_parser<IsRequest>& instance, std::string text,
    std::string& rest, size_t step=max_size_t)
{
    code ec{};
    std::string buffer{};
    while (!text.empty() || !buffer.empty())
    {
        const auto size = std::min(step, text.size());
        buffer += text.substr(zero, size);
        text.erase(zero, size);
        const auto used = instance.put(
            io::const_buffer{ buffer.data(), buffer.size() }, ec);
        buffer.erase(zero, used);
        if (instance.is_done() || (ec && ec != http_error_t::need_more))
            break;

        if (text.empty() && (is_zero(used) || buffer.empty()))
            break;
    }

    rest = buffer + text;
    return ec;
}

// requests

BC_AUTO_TEST_CASE(http_parser__put__content_length__expected)
{
    string_parser<true> instance{};
    std::string rest{};
    const auto ec = feed(instance, "POST /x?y HTTP/1.1\r\nHost: h\r\nContent-Length: 5\r\n\r\nhelloGET", rest);
    BC_REQUIRE(!ec);
    BC_REQUIRE(instance.is_done());
    BC_REQUIRE(instance.get().method() == verb::post);
    BC_REQUIRE_EQUAL(instance.get().target(), "/x?y");
    BC_REQUIRE_EQUAL(instance.get().version(), version_1_1);
    BC_REQUIRE_EQUAL(instance.get()[field::host], "h");
    BC_REQUIRE_EQUAL(instance.get().body(), "hello");
    BC_REQUIRE_EQUAL(rest, "GET");
}

BC_AUTO_TEST_CASE(http_parser__put__bytewise_chunked__expected)
{
    string_parser<true> instance{};
    std::string rest{};
    const auto ec = feed(instance, "POST / HTTP/1.1\r\nTransfer-Encoding: gzip, chunked\r\n\r\n3;a=\"b\"\r\nabc\r\n2\r\nde\r\n0\r\nTrailer: t\r\n\r\n", rest, 1);
    BC_REQUIRE(!ec);
    BC_REQUIRE(instance.is_done());
    BC_REQUIRE_EQUAL(instance.get().body(), "abcde");
    BC_REQUIRE(instance.get().find("Trailer") == instance.get().end());
    BC_REQUIRE(rest.empty());
}

BC_AUTO_TEST_CASE(http_parser__put__no_framing_request__no_body)
{
    string_parser<true> instance{};
    std::string rest{};
    const auto ec = feed(instance, "GET / HTTP/1.0\r\n\r\nextra", rest);
    BC_REQUIRE(!ec);
    BC_REQUIRE(instance.is_done());
    BC_REQUIRE(instance.get().body().empty());
    BC_REQUIRE_EQUAL(rest, "extra");
}

BC_AUTO_TEST_CASE(http_parser__put__header__pauses)
{
    string_parser<true> instance{};
    const std::string text{ "POST / HTTP/1.1\r\nContent-Length: 2\r\n\r\nab" };
    code ec{};
    const auto used = instance.put(io::const_buffer{ text.data(), text.size() }, ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(instance.is_header_done());
    BC_REQUIRE(!instance.is_done());
    BC_REQUIRE_EQUAL(used, text.size() - 2u);
}

BC_AUTO_TEST_CASE(http_parser__put__folded_value__space_joined)
{
    string_parser<true> instance{};
    std::string rest{};
    const auto ec = feed(instance, "GET / HTTP/1.1\r\nX-A: one \r\n  two\r\n\r\n", rest);
    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(instance.get()["X-A"], "one two");
}

BC_AUTO_TEST_CASE(http_parser__put__errors__expected)
{
    const std::vector<std::pair<std::string, http_error_t>> vectors
    {
        { "G@T / HTTP/1.1\r\n\r\n", http_error_t::bad_method },
        { "GET  / HTTP/1.1\r\n\r\n", http_error_t::bad_target },
        { "GET / HTTP/2.0\r\n\r\n", http_error_t::bad_version },
        { "GET / HTTP/1.1\r\nBad Name: x\r\n\r\n", http_error_t::bad_field },
        { "GET / HTTP/1.1\r\nX: a\x01" "b\r\n\r\n", http_error_t::bad_value },
        { "GET / HTTP/1.1\r\nX: a\rb\r\n\r\n", http_error_t::bad_line_ending },
        { "GET / HTTP/1.1\r\nContent-Length: x\r\n\r\n", http_error_t::bad_content_length },
        { "GET / HTTP/1.1\r\nContent-Length: 1\r\nContent-Length: 2\r\n\r\n", http_error_t::multiple_content_length },
        { "GET / HTTP/1.1\r\nTransfer-Encoding: chunked\r\nContent-Length: 1\r\n\r\n", http_error_t::bad_content_length },
        { "GET / HTTP/1.1\r\nContent-Length: 1\r\nTransfer-Encoding: chunked\r\n\r\n", http_error_t::bad_transfer_encoding },
        { "GET / HTTP/1.1\r\nTransfer-Encoding: gzip\r\n\r\n", http_error_t::bad_transfer_encoding },
        { "GET / HTTP/1.1\r\nTransfer-Encoding: chunked, gzip\r\n\r\n", http_error_t::bad_transfer_encoding },
        { "GET / HTTP/1.1\r\nTransfer-Encoding: gzip\r\nTransfer-Encoding: chunked\r\n\r\n", http_error_t::bad_transfer_encoding },
        { "GET / HTTP/1.1\r\nTransfer-Encoding: gzip\r\nContent-Length: 1\r\n\r\n", http_error_t::bad_content_length },
        { "GET / HTTP/1.1\r\nConnection: a b\r\n\r\n", http_error_t::bad_value },
        { "GET / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\nx\r\n", http_error_t::bad_chunk },
        { "GET / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n1 \r\na\r\n0\r\n\r\n", http_error_t::bad_chunk_extension },
        { "GET / HTTP/1.1\r\nTransfer-Encoding: chunked\r\n\r\n1\r\naXY\r\n", http_error_t::bad_chunk }
    };

    for (const auto& [text, expected]: vectors)
    {
        string_parser<true> instance{};
        std::string rest{};
        BC_REQUIRE_MESSAGE(feed(instance, text, rest) == expected, text);
    }
}

BC_AUTO_TEST_CASE(http_parser__put__header_limit__exceeded)
{
    string_parser<true> instance{};
    instance.header_limit(32);
    std::string rest{};
    BC_REQUIRE(feed(instance, "GET / HTTP/1.1\r\nX-Long: " + std::string(64, 'x') + "\r\n\r\n", rest) == http_error_t::header_limit);
}

BC_AUTO_TEST_CASE(http_parser__put__body_limit__exceeded)
{
    string_parser<true> instance{};
    instance.body_limit(4);
    std::string rest{};
    BC_REQUIRE(feed(instance, "POST / HTTP/1.1\r\nContent-Length: 5\r\n\r\nhello", rest) == http_error_t::body_limit);
}

BC_AUTO_TEST_CASE(http_parser__put_eof__incomplete__expected)
{
    string_parser<true> empty{};
    code ec{};
    empty.put_eof(ec);
    BC_REQUIRE(ec == http_error_t::end_of_stream);

    string_parser<true> partial{};
    std::string rest{};
    feed(partial, "GET / HTTP/1.1\r\nHo", rest);
    partial.put_eof(ec);
    BC_REQUIRE(ec == http_error_t::partial_message);
}

// responses

BC_AUTO_TEST_CASE(http_parser__put__response_to_eof__body_on_eof)
{
    string_parser<false> instance{};
    std::string rest{};
    feed(instance, "HTTP/1.0 200 Fine\r\n\r\nsome body", rest);
    BC_REQUIRE(!instance.is_done());
    BC_REQUIRE_EQUAL(instance.get().reason(), "Fine");

    code ec{};
    instance.put_eof(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(instance.is_done());
    BC_REQUIRE_EQUAL(instance.get().body(), "some body");
}

BC_AUTO_TEST_CASE(http_parser__put__response_encoded__body_on_eof)
{
    string_parser<false> instance{};
    std::string rest{};
    feed(instance, "HTTP/1.1 200 OK\r\nTransfer-Encoding: gzip\r\n\r\nsome body", rest);
    BC_REQUIRE(!instance.is_done());

    code ec{};
    instance.put_eof(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(instance.is_done());
    BC_REQUIRE_EQUAL(instance.get().body(), "some body");
}

BC_AUTO_TEST_CASE(http_parser__put__bodiless_status__no_body)
{
    string_parser<false> instance{};
    std::string rest{};
    const auto ec = feed(instance, "HTTP/1.1 204 No Content\r\nContent-Length: 5\r\n\r\nhello", rest);
    BC_REQUIRE(!ec);
    BC_REQUIRE(instance.is_done());
    BC_REQUIRE(instance.get().body().empty());
    BC_REQUIRE_EQUAL(rest, "hello");
}

BC_AUTO_TEST_CASE(http_parser__put__bad_status__expected)
{
    string_parser<false> instance{};
    std::string rest{};
    BC_REQUIRE(feed(instance, "HTTP/1.1 20 OK\r\n\r\n", rest) == http_error_t::bad_status);
}

// body_parser

BC_AUTO_TEST_CASE(http_parser__body_parser__streamed_split_rpc__need_more_then_done)
{
    const std::string_view first{ R"({"jsonrpc":"2.0","id":1,)" };
    const std::string_view second{ R"("method":"x","params":[]})" };
    request value{};
    value.body() = network::rpc::request{};
    body_parser<true, body> instance{ value, true };
    BC_REQUIRE(instance.is_header_done());

    code ec{};
    BC_REQUIRE_EQUAL(instance.put(io::const_buffer{ first.data(), first.size() }, ec), first.size());
    BC_REQUIRE(ec == http_error_t::need_more);
    BC_REQUIRE(!instance.is_done());
    BC_REQUIRE_EQUAL(instance.put(io::const_buffer{ second.data(), second.size() }, ec), second.size());
    BC_REQUIRE(!ec);
    BC_REQUIRE(instance.is_done());
    BC_REQUIRE_EQUAL(value.body().get<network::rpc::request>().message.method, "x");
}

BC_AUTO_TEST_CASE(http_parser__body_parser__framed_incomplete_rpc__json_incomplete)
{
    const std::string_view text{ R"({"id":1)" };
    request value{};
    value.body() = network::rpc::request{};
    body_parser<true, body> instance{ value, false };

    code ec{};
    BC_REQUIRE_EQUAL(instance.put(io::const_buffer{ text.data(), text.size() }, ec), text.size());
    BC_REQUIRE(ec == to_json_code(json_error_t::json_incomplete));
    BC_REQUIRE(!instance.is_done());
}

BC_AUTO_TEST_CASE(http_parser__body_parser__over_limit__body_limit)
{
    const std::string_view text{ R"({"jsonrpc":"2.0","id":1,"method":"x","params":[]})" };
    request value{};
    value.body() = network::rpc::request{};
    body_parser<true, body> instance{ value, true };
    instance.body_limit(4);

    code ec{};
    instance.put(io::const_buffer{ text.data(), text.size() }, ec);
    BC_REQUIRE(ec == http_error_t::body_limit);
    BC_REQUIRE(!instance.is_done());
}

BC_AUTO_TEST_CASE(http_parser__body_parser__put_eof__success_not_done)
{
    request value{};
    value.body() = network::rpc::request{};
    body_parser<true, body> instance{ value, true };

    auto ec = to_http_code(http_error_t::end_of_stream);
    instance.put_eof(ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(!instance.is_done());
}

// serializer

BC_AUTO_TEST_CASE(http_parser__serializer__chunked__round_trip)
{
    message<false, string_body> out{ status::ok, version_1_1 };
    out.set(field::server, "test");
    out.body() = "payload";
    out.prepare_payload();

    code ec{};
    std::string wire{};
    serializer<false, string_body> instance{ out };
    while (!instance.is_done())
        for (const auto& buffer: instance.next(ec))
            wire.append(system::pointer_cast<const char>(buffer.data()), buffer.size());

    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(wire, "HTTP/1.1 200 OK\r\nServer: test\r\nTransfer-Encoding: chunked\r\n\r\n7\r\npayload\r\n0\r\n\r\n");

    string_parser<false> parse{};
    std::string rest{};
    BC_REQUIRE(!feed(parse, wire, rest));
    BC_REQUIRE_EQUAL(parse.get().body(), "payload");
}

BC_AUTO_TEST_CASE(http_parser__serializer__content_length__raw_body)
{
    message<true, string_body> out{ verb::put, "/a", version_1_1 };
    out.body() = "abc";
    out.content_length(3u);

    code ec{};
    std::string wire{};
    serializer<true, string_body> instance{ out };
    while (!instance.is_done())
        for (const auto& buffer: instance.next(ec))
            wire.append(system::pointer_cast<const char>(buffer.data()), buffer.size());

    BC_REQUIRE_EQUAL(wire, "PUT /a HTTP/1.1\r\nContent-Length: 3\r\n\r\nabc");
}

// body_serializer

BC_AUTO_TEST_CASE(http_parser__body_serializer__string__body_only)
{
    message<false, string_body> out{ status::ok, version_1_1 };
    out.set(field::server, "test");
    out.body() = "payload";

    code ec{};
    std::string wire{};
    body_serializer<false, string_body> instance{ out };
    while (!instance.is_done())
        for (const auto& buffer: instance.next(ec))
            wire.append(system::pointer_cast<const char>(buffer.data()), buffer.size());

    BC_REQUIRE(!ec);
    BC_REQUIRE_EQUAL(wire, "payload");
    BC_REQUIRE(instance.next(ec).empty());
}

BC_AUTO_TEST_CASE(http_parser__body_serializer__data__binary)
{
    response out{};
    out.body() = chunk_body::value_type{};
    const body_serializer<false, body> instance{ out };
    BC_REQUIRE(instance.binary());
}

BC_AUTO_TEST_CASE(http_parser__body_serializer__string__text)
{
    response out{};
    out.body() = string_body::value_type{};
    const body_serializer<false, body> instance{ out };
    BC_REQUIRE(!instance.binary());
}

BC_AUTO_TEST_CASE(http_parser__chunk_header__sizes__lower_hex)
{
    BC_REQUIRE_EQUAL(chunk_header(0), "0\r\n");
    BC_REQUIRE_EQUAL(chunk_header(255), "ff\r\n");
    BC_REQUIRE_EQUAL(chunk_header(4096), "1000\r\n");
}

BC_AUTO_TEST_SUITE_END()
