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

BC_AUTO_TEST_SUITE(http_codings_tests)

using namespace network::http;
using namespace network::error;

static fields accepting(const std::string& value)
{
    fields out{};
    out.set(field::accept_encoding, value);
    return out;
}

// coding

BC_AUTO_TEST_CASE(http_codings__to_coding__names__expected)
{
    BC_REQUIRE(to_coding("identity") == coding::identity);
    BC_REQUIRE(to_coding("GZIP") == coding::gzip);
    BC_REQUIRE(to_coding("x-gzip") == coding::gzip);
    BC_REQUIRE(to_coding("Deflate") == coding::deflate);
    BC_REQUIRE(to_coding("br") == coding::unknown);
    BC_REQUIRE(to_coding("") == coding::unknown);
}

BC_AUTO_TEST_CASE(http_codings__to_string__codings__expected)
{
    BC_REQUIRE_EQUAL(to_string(coding::identity), "identity");
    BC_REQUIRE_EQUAL(to_string(coding::gzip), "gzip");
    BC_REQUIRE_EQUAL(to_string(coding::deflate), "deflate");
    BC_REQUIRE(to_string(coding::unknown).empty());
}

// to_codings

BC_AUTO_TEST_CASE(http_codings__to_codings__none__empty)
{
    code ec{};
    BC_REQUIRE(to_codings(fields{}, ec).empty());
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(http_codings__to_codings__content_and_transfer__ordered)
{
    fields header{};
    header.set(field::content_encoding, "deflate, identity");
    header.insert(field::content_encoding, "gzip");
    header.set(field::transfer_encoding, "gzip, chunked");

    code ec{};
    const auto value = to_codings(header, ec);
    BC_REQUIRE(!ec);
    BC_REQUIRE(value.content == std::vector<coding>({ coding::deflate, coding::gzip }));
    BC_REQUIRE(value.transfer == std::vector<coding>({ coding::gzip }));
}

BC_AUTO_TEST_CASE(http_codings__to_codings__unknown_content__bad_content_encoding)
{
    fields header{};
    header.set(field::content_encoding, "gzip, br");

    code ec{};
    BC_REQUIRE(to_codings(header, ec).empty());
    BC_REQUIRE(ec == http_error_t::bad_content_encoding);
}

BC_AUTO_TEST_CASE(http_codings__to_codings__unknown_transfer__bad_transfer_encoding)
{
    fields header{};
    header.set(field::transfer_encoding, "compress, chunked");

    code ec{};
    BC_REQUIRE(to_codings(header, ec).empty());
    BC_REQUIRE(ec == http_error_t::bad_transfer_encoding);
}

BC_AUTO_TEST_CASE(http_codings__to_codings__chunked_not_last__bad_transfer_encoding)
{
    fields header{};
    header.set(field::transfer_encoding, "chunked, gzip");

    code ec{};
    to_codings(header, ec);
    BC_REQUIRE(ec == http_error_t::bad_transfer_encoding);
}

// encode/decode

BC_AUTO_TEST_CASE(http_codings__decode__encoded__round_trip)
{
    const auto body = system::to_chunk(std::string{ "hello hello hello hello world" });
    const codings value{ { coding::deflate, coding::gzip }, { coding::gzip } };

    code ec{};
    BC_REQUIRE_EQUAL(decode(encode(body, value), value, 0, ec), body);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(http_codings__encode__empty_codings__unchanged)
{
    const auto body = system::to_chunk(std::string{ "hello" });
    BC_REQUIRE_EQUAL(encode(body, codings{}), body);
}

BC_AUTO_TEST_CASE(http_codings__decode__bad_content__bad_content_encoding)
{
    const auto body = system::to_chunk(std::string{ "hello" });
    const codings value{ { coding::gzip }, {} };

    code ec{};
    BC_REQUIRE(decode(body, value, 0, ec).empty());
    BC_REQUIRE(ec == http_error_t::bad_content_encoding);
}

BC_AUTO_TEST_CASE(http_codings__decode__bad_transfer__bad_transfer_encoding)
{
    const auto body = system::to_chunk(std::string{ "hello" });
    const codings value{ {}, { coding::deflate } };

    code ec{};
    BC_REQUIRE(decode(body, value, 0, ec).empty());
    BC_REQUIRE(ec == http_error_t::bad_transfer_encoding);
}

BC_AUTO_TEST_CASE(http_codings__decode__limit_exceeded__body_limit)
{
    const system::data_chunk body(1000, 'a');
    const codings value{ { coding::gzip }, {} };

    code ec{};
    BC_REQUIRE(decode(encode(body, value), value, 999, ec).empty());
    BC_REQUIRE(ec == http_error_t::body_limit);
}

BC_AUTO_TEST_CASE(http_codings__decode__limit_equal__expected)
{
    const system::data_chunk body(1000, 'a');
    const codings value{ { coding::gzip }, { coding::deflate } };

    code ec{};
    BC_REQUIRE_EQUAL(decode(encode(body, value), value, 1000, ec), body);
    BC_REQUIRE(!ec);
}

BC_AUTO_TEST_CASE(http_codings__decoder__bytewise__expected)
{
    const auto body = system::to_chunk(std::string{ "hello hello hello hello world" });
    const codings value{ { coding::deflate }, { coding::gzip } };
    const auto coded = encode(body, value);

    code ec{};
    system::data_chunk out{};
    decoder coder{ value, 0 };
    std::for_each(coded.begin(), coded.end(), [&](uint8_t byte) NOEXCEPT
    {
        size_t consumed{};
        coder.write(out, consumed, system::data_chunk{ byte }, 0, ec);
    });

    BC_REQUIRE(!ec);
    BC_REQUIRE(coder.done());
    BC_REQUIRE_EQUAL(out, body);
}

BC_AUTO_TEST_CASE(http_codings__encoder__parts__encode)
{
    const system::data_chunk body(70000, 'e');
    const codings value{ { coding::gzip }, {} };
    const system::data_slice left(body.data(), std::next(body.data(), 30000));
    const system::data_slice right(std::next(body.data(), 30000), std::next(body.data(), body.size()));

    system::data_chunk out{};
    encoder coder{ value };
    coder.write(out, left);
    coder.write(out, right);
    coder.flush(out);
    BC_REQUIRE_EQUAL(out, encode(body, value));
}

// accepted_coding

BC_AUTO_TEST_CASE(http_codings__accepted_coding__no_field__identity)
{
    BC_REQUIRE(accepted_coding(fields{}) == coding::identity);
}

BC_AUTO_TEST_CASE(http_codings__accepted_coding__vectors__expected)
{
    const std::vector<std::pair<std::string, coding>> vectors
    {
        { "", coding::identity },
        { "gzip", coding::gzip },
        { "x-gzip", coding::gzip },
        { "deflate", coding::deflate },
        { "deflate, gzip", coding::gzip },
        { "gzip;q=0.5, deflate", coding::deflate },
        { "gzip;q=0.5", coding::gzip },
        { "gzip;q=0.5, identity", coding::identity },
        { "gzip;q=0", coding::identity },
        { "br", coding::identity },
        { "*", coding::gzip },
        { "*;q=0.1, deflate", coding::deflate },
        { "identity;q=0, *;q=0", coding::unknown },
        { "*;q=0", coding::unknown },
        { "*;q=0, gzip;q=0.001", coding::gzip },
        { "identity;q=0", coding::unknown },
        { "GZIP; Q=1.000", coding::gzip },
        { "gzip;q=1.5, deflate;q=0.1", coding::deflate },
        { "gzip;q=0.1234, deflate;q=0.1", coding::deflate },
        { "gzip;q=x", coding::identity }
    };

    for (const auto& [text, expected]: vectors)
    {
        BC_REQUIRE_MESSAGE(accepted_coding(accepting(text)) == expected, text);
    }
}

BC_AUTO_TEST_CASE(http_codings__accepted_coding__multiple_fields__combined)
{
    fields request{};
    request.set(field::accept_encoding, "identity;q=0.5");
    request.insert(field::accept_encoding, "deflate");
    BC_REQUIRE(accepted_coding(request) == coding::deflate);
}

BC_AUTO_TEST_SUITE_END()
