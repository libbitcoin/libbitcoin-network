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

BOOST_AUTO_TEST_SUITE(tls_server_hello_tests)

using namespace bc::system;
using namespace tls;
using writer = network::tls::writer;

const auto server_secret = base16_array("c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721");
const auto share = base16_chunk("99381de560e4bd43d23d8e435a7dbafeb3c06e51c13cae4d5413691e529aaf2c");

// ClientHello fields, each a complete extension or field encoding.
struct hello
{
    size_t session{};
    data_chunk suites{ base16_chunk("13011303") };
    data_chunk compression{ base16_chunk("00") };
    bool versions{ true };
    bool groups{ true };
    bool algorithms{ true };
    bool shares{ true };
    bool early{};
    bool duplicate{};
    data_chunk version_list{ base16_chunk("0304") };
    data_chunk group_list{ base16_chunk("001d0017") };
    data_chunk algorithm_list{ base16_chunk("04030503") };
    data_chunk key_share{ splice(base16_chunk("001d0020"), share) };
};

static void extension(writer& out, uint16_t type, const data_chunk& data)
{
    out.write_16(type);
    out.write_vector_16(data);
}

static data_chunk vector_8(const data_chunk& data)
{
    writer out{};
    out.write_vector_8(data);
    return out.data();
}

static data_chunk vector_16(const data_chunk& data)
{
    writer out{};
    out.write_vector_16(data);
    return out.data();
}

static data_chunk record_of(uint8_t type, const data_chunk& content)
{
    writer out{};
    out.write_8(type);
    out.write_16(legacy_version);
    out.write_vector_16(content);
    return out.data();
}

static data_chunk encode(const hello& value)
{
    writer extensions{};
    if (value.versions) extension(extensions, extension::supported_versions, vector_8(value.version_list));
    if (value.groups) extension(extensions, extension::supported_groups, vector_16(value.group_list));
    if (value.algorithms) extension(extensions, extension::signature_algorithms, vector_16(value.algorithm_list));
    if (value.shares) extension(extensions, extension::key_share, vector_16(value.key_share));
    if (value.early) extension(extensions, extension::early_data, {});
    if (value.duplicate) extension(extensions, extension::supported_groups, vector_16(value.group_list));

    writer body{};
    body.write_16(legacy_version);
    body.write_bytes(data_chunk(32, 0x42));
    body.write_vector_8(data_chunk(value.session, 0x00));
    body.write_vector_16(value.suites);
    body.write_vector_8(value.compression);
    body.write_vector_16(extensions.data());

    writer message{};
    message.write_8(handshake::client_hello);
    message.write_vector_24(body.data());
    return record_of(content::handshake, message.data());
}

struct setup
{
    setup()
    {
        x509::subject subject{};
        subject.common_name = "server";
        subject.not_before = 1735689600;
        subject.not_after = 2524608000;
        data_chunk der{};
        x509::build_self_signed(der, server_secret, subject);
        context.set_chain(x509::encode_certificate(der));
        context.set_key(x509::encode_private_key(server_secret), {});
    }

    tls::context context{};
};

static uint8_t failure_of(const hello& value)
{
    const setup instance{};
    tls::server server{ instance.context };
    server.receive(encode(value));
    return server.is_failed() ? server.failure() : alert::close_notify;
}

BOOST_AUTO_TEST_CASE(tls_server_hello__valid__accepted)
{
    BOOST_REQUIRE_EQUAL(failure_of({}), alert::close_notify);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__missing_versions__protocol_version)
{
    hello value{};
    value.versions = false;
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::protocol_version);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__tls12_version_only__protocol_version)
{
    hello value{};
    value.version_list = base16_chunk("0303");
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::protocol_version);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__missing_algorithms__missing_extension)
{
    hello value{};
    value.algorithms = false;
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::missing_extension);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__missing_groups__missing_extension)
{
    hello value{};
    value.groups = false;
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::missing_extension);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__missing_key_share__missing_extension)
{
    hello value{};
    value.shares = false;
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::missing_extension);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__no_p256_signature__handshake_failure)
{
    hello value{};
    value.algorithm_list = base16_chunk("0503");
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::handshake_failure);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__no_x25519_group__handshake_failure)
{
    hello value{};
    value.group_list = base16_chunk("0017");
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::handshake_failure);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__duplicate_extension__illegal_parameter)
{
    hello value{};
    value.duplicate = true;
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::illegal_parameter);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__compression__illegal_parameter)
{
    hello value{};
    value.compression = base16_chunk("01");
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::illegal_parameter);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__long_session__decode_error)
{
    hello value{};
    value.session = 33;
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__odd_suites__decode_error)
{
    hello value{};
    value.suites = base16_chunk("130113");
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__malformed_groups__decode_error)
{
    hello value{};
    value.group_list = base16_chunk("001d00");
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__malformed_share__decode_error)
{
    hello value{};
    value.key_share = base16_chunk("001d00ff");
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__short_share__illegal_parameter)
{
    hello value{};
    value.key_share = base16_chunk("001d000101");
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::illegal_parameter);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__zero_share__illegal_parameter)
{
    hello value{};
    value.key_share = splice(base16_chunk("001d0020"), data_chunk(32, 0x00));
    BOOST_REQUIRE_EQUAL(failure_of(value), alert::illegal_parameter);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__retry_without_share__illegal_parameter)
{
    const setup instance{};
    tls::server server{ instance.context };
    hello value{};
    value.key_share = {};
    BOOST_REQUIRE(server.receive(encode(value)));
    BOOST_REQUIRE(!server.receive(encode(value)));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::illegal_parameter);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__retry_with_early_data__illegal_parameter)
{
    const setup instance{};
    tls::server server{ instance.context };
    hello first{};
    first.key_share = {};
    hello second{};
    second.early = true;
    BOOST_REQUIRE(server.receive(encode(first)));
    BOOST_REQUIRE(!server.receive(encode(second)));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::illegal_parameter);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__retry_change_cipher_spec__ignored)
{
    const setup instance{};
    tls::server server{ instance.context };
    hello value{};
    value.key_share = {};
    BOOST_REQUIRE(server.receive(encode(value)));
    BOOST_REQUIRE(server.receive(record_of(content::change_cipher_spec, base16_chunk("01"))));
    BOOST_REQUIRE(!server.is_failed());
}

// records

BOOST_AUTO_TEST_CASE(tls_server_hello__change_cipher_spec_first__unexpected_message)
{
    const setup instance{};
    tls::server server{ instance.context };
    BOOST_REQUIRE(!server.receive(record_of(content::change_cipher_spec, base16_chunk("01"))));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__empty_handshake__unexpected_message)
{
    const setup instance{};
    tls::server server{ instance.context };
    BOOST_REQUIRE(!server.receive(record_of(content::handshake, {})));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__long_alert__decode_error)
{
    const setup instance{};
    tls::server server{ instance.context };
    BOOST_REQUIRE(!server.receive(record_of(content::alert, base16_chunk("020a00"))));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__unknown_content__unexpected_message)
{
    const setup instance{};
    tls::server server{ instance.context };
    BOOST_REQUIRE(!server.receive(record_of(24, base16_chunk("00"))));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__oversized_message__decode_error)
{
    const setup instance{};
    tls::server server{ instance.context };
    BOOST_REQUIRE(!server.receive(record_of(content::handshake, base16_chunk("01ffffff"))));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::decode_error);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__unexpected_message_type__unexpected_message)
{
    const setup instance{};
    tls::server server{ instance.context };
    BOOST_REQUIRE(!server.receive(record_of(content::handshake, base16_chunk("14000000"))));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__trailing_message__unexpected_message)
{
    const setup instance{};
    tls::server server{ instance.context };
    const auto hello_record = encode({});
    const data_chunk message(std::next(hello_record.begin(), record_header_size), hello_record.end());
    BOOST_REQUIRE(!server.receive(record_of(content::handshake, splice(message, base16_chunk("14000000")))));
    BOOST_REQUIRE_EQUAL(server.failure(), alert::unexpected_message);
}

BOOST_AUTO_TEST_CASE(tls_server_hello__failed__further_receive_false)
{
    const setup instance{};
    tls::server server{ instance.context };
    BOOST_REQUIRE(!server.receive(record_of(24, base16_chunk("00"))));
    BOOST_REQUIRE(!server.receive(encode({})));
    BOOST_REQUIRE(!server.write(base16_chunk("00")));
}

BOOST_AUTO_TEST_SUITE_END()
