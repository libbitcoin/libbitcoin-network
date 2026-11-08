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

BC_AUTO_TEST_SUITE(tls_context_tests)

using namespace bc::system;
using namespace tls;

const auto key1 = base16_array("c9afa9d845ba75166b5c215767b1d6934e50c3db36e89b127b8a622b120f6721");
const auto key2 = base16_array("0000000000000000000000000000000000000000000000000000000000000002");
const auto key384 = base16_array("6bfc6ce1ef1beb42ecb60360242e9efab64189b1277c562b58eedad3b3586ff590179425f623bd9532ebd3091eb3cbb5");

// The secp384r1 certificate of key384 (openssl).
const std::string chain384
{
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB6zCCAXGgAwIBAgIUJtHsAFXdYgGYBXLxCgPAvtfK/PcwCgYIKoZIzj0EAwMw\n"
    "FDESMBAGA1UEAwwJY2xpZW50Mzg0MB4XDTI2MDEwMTAwMDAwMFoXDTQ5MTIzMTIz\n"
    "NTk1OVowFDESMBAGA1UEAwwJY2xpZW50Mzg0MHYwEAYHKoZIzj0CAQYFK4EEACID\n"
    "YgAECOQks2KFZdRtQWoliPUFV9bhyFan2z1R+sutT7q3fTsTHK2fUZxA9HZvriMZ\n"
    "HEKEIzdITbaBVXfFUnhH5W3UiU2P5CmOC6vwREAgpthnG8QVxJ+RqT7snfceDa7T\n"
    "l28io4GDMIGAMB0GA1UdDgQWBBTJQ+ViudEUUbSJ+d34rwXXN4emXjAfBgNVHSME\n"
    "GDAWgBTJQ+ViudEUUbSJ+d34rwXXN4emXjAPBgNVHRMBAf8EBTADAQH/MA4GA1Ud\n"
    "DwEB/wQEAwIChDAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwCgYIKoZI\n"
    "zj0EAwMDaAAwZQIxAKw8qvXbX8/kRuCJQPuDpyVj5vHkOQ/T/hGqnJiXihrBvSQc\n"
    "tWKiP+AIuqd+JpCnMgIwZ21W5QpX7QNqcFdoUbWW1BxaLDekhmmKPa6+SIte9H3V\n"
    "6kxQ4IMBYC3p0j3En1DQ\n"
    "-----END CERTIFICATE-----\n"
};

// An RSA certificate signed sha256WithRSAEncryption (openssl).
const std::string rsa_certificate
{
    "-----BEGIN CERTIFICATE-----\n"
    "MIIB+DCCAWGgAwIBAgIUGEtUwKm5cw6/L2Mu5zX3kJDM2NswDQYJKoZIhvcNAQEL\n"
    "BQAwDjEMMAoGA1UEAwwDcnNhMB4XDTI2MTEwODIwNTk0OFoXDTM2MTEwNTIwNTk0\n"
    "OFowDjEMMAoGA1UEAwwDcnNhMIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDQ\n"
    "K3+c7v4YFdHLr1TVAn64I+udck1ywexWhwwC02mmxJzzd+783qE+ApPjutwMPPSN\n"
    "74HF9+OgklEduv0oQCYCZm7nrEheIcZU70KZiHrAX2/sPEzGQTvQhOe5Y/7BDnbM\n"
    "Mbb7kLw2ksL4aZegdweuEPcokFmeqlOsIRxeLHUnUwIDAQABo1MwUTAdBgNVHQ4E\n"
    "FgQUqHLT7YsNVZrcaGbiIkjSzJd3JC8wHwYDVR0jBBgwFoAUqHLT7YsNVZrcaGbi\n"
    "IkjSzJd3JC8wDwYDVR0TAQH/BAUwAwEB/zANBgkqhkiG9w0BAQsFAAOBgQBzFvPu\n"
    "X6EplpbNGenBEKnr3Qa+14GmMej+AXtQEOwlAb4rc1Ko4hvAXictrZhXh0uTgia+\n"
    "R0PPlwIogjfcNlJu2ZWd2VB04g/vz/XfloS3S1uW6u177Ot0DCLVOd4fyvU+CNaa\n"
    "1/I4KSzSmWAy+3t3e6TDfNze8rZfMgXalZQeWQ==\n"
    "-----END CERTIFICATE-----\n"
};

const std::string malformed_certificate
{
    "-----BEGIN CERTIFICATE-----\n"
    "MAAA\n"
    "-----END CERTIFICATE-----\n"
};

static std::string certificate_of(const x509::secret& key)
{
    x509::subject subject{};
    subject.common_name = "context";
    subject.not_before = 1735689600;
    subject.not_after = 2524608000;

    data_chunk der{};
    x509::build_self_signed(der, key, subject);
    return x509::encode_certificate(der);
}

BC_AUTO_TEST_CASE(tls_context__construct__default__not_ready)
{
    const tls::context context{};
    BC_REQUIRE(!context.is_ready());
    BC_REQUIRE(context.certificates().empty());
    BC_REQUIRE(context.anchors().empty());
    BC_REQUIRE(!context.request());
    BC_REQUIRE(!context.require());
    BC_REQUIRE(context.time() > 1735689600u);
}

BC_AUTO_TEST_CASE(tls_context__set_chain_and_key__matching__ready)
{
    tls::context context{};
    BC_REQUIRE(context.set_chain(certificate_of(key1)));
    BC_REQUIRE(!context.is_ready());
    BC_REQUIRE(context.set_key(x509::encode_private_key(key1), {}));
    BC_REQUIRE(context.is_ready());
    BC_REQUIRE_EQUAL(context.certificates().size(), 1u);
    BC_REQUIRE(context.curve() == x509::curve::secp256r1);
    BC_REQUIRE_EQUAL(context.key(), key1);
}

BC_AUTO_TEST_CASE(tls_context__set_key_then_chain__matching__ready)
{
    tls::context context{};
    BC_REQUIRE(context.set_key(x509::encode_private_key(key1), {}));
    BC_REQUIRE(context.set_chain(certificate_of(key1)));
    BC_REQUIRE(context.is_ready());
}

BC_AUTO_TEST_CASE(tls_context__set_key__mismatched__false_unchanged)
{
    tls::context context{};
    BC_REQUIRE(context.set_chain(certificate_of(key1)));
    BC_REQUIRE(!context.set_key(x509::encode_private_key(key2), {}));
    BC_REQUIRE(!context.is_ready());
}

BC_AUTO_TEST_CASE(tls_context__set_chain__mismatched__false_cleared)
{
    tls::context context{};
    BC_REQUIRE(context.set_key(x509::encode_private_key(key1), {}));
    BC_REQUIRE(!context.set_chain(certificate_of(key2)));
    BC_REQUIRE(context.certificates().empty());
}

BC_AUTO_TEST_CASE(tls_context__set_chain_and_key__malformed__false)
{
    tls::context context{};
    BC_REQUIRE(!context.set_chain("not a certificate"));
    BC_REQUIRE(!context.set_key("not a key", {}));
    BC_REQUIRE(!context.add_anchors("not a certificate"));
}

BC_AUTO_TEST_CASE(tls_context__set_chain_and_key__secp384r1__ready)
{
    tls::context context{};
    BC_REQUIRE(context.set_chain(chain384));
    BC_REQUIRE(context.set_key(x509::encode_private_key(key384), {}));
    BC_REQUIRE(context.is_ready());
    BC_REQUIRE(context.curve() == x509::curve::secp384r1);
    BC_REQUIRE_EQUAL(context.key384(), key384);
}

BC_AUTO_TEST_CASE(tls_context__set_key__secp384r1_for_secp256r1_chain__false_unchanged)
{
    tls::context context{};
    BC_REQUIRE(context.set_chain(certificate_of(key1)));
    BC_REQUIRE(!context.set_key(x509::encode_private_key(key384), {}));
    BC_REQUIRE(!context.is_ready());
}

BC_AUTO_TEST_CASE(tls_context__set_key__secp256r1_for_secp384r1_chain__false_unchanged)
{
    tls::context context{};
    BC_REQUIRE(context.set_chain(chain384));
    BC_REQUIRE(!context.set_key(x509::encode_private_key(key1), {}));
    BC_REQUIRE(!context.is_ready());
}

BC_AUTO_TEST_CASE(tls_context__set_chain__rsa_trailing__passed_through)
{
    x509::pems leaf{};
    x509::pems trailing{};
    const auto chain = certificate_of(key1) + rsa_certificate;
    BC_REQUIRE(x509::decode_pem(leaf, chain));
    BC_REQUIRE(x509::decode_pem(trailing, rsa_certificate));
    tls::context context{};
    BC_REQUIRE(context.set_chain(chain));
    BC_REQUIRE(context.set_key(x509::encode_private_key(key1), {}));
    BC_REQUIRE(context.is_ready());
    BC_REQUIRE_EQUAL(context.certificates().size(), 2u);
    BC_REQUIRE_EQUAL(context.certificates().front(), leaf.front().data);
    BC_REQUIRE_EQUAL(context.certificates().back(), trailing.front().data);
}

BC_AUTO_TEST_CASE(tls_context__set_chain__rsa_leaf__false)
{
    tls::context context{};
    BC_REQUIRE(!context.set_chain(rsa_certificate + certificate_of(key1)));
    BC_REQUIRE(context.certificates().empty());
}

BC_AUTO_TEST_CASE(tls_context__set_chain__malformed_trailing__false)
{
    tls::context context{};
    BC_REQUIRE(!context.set_chain(certificate_of(key1) + malformed_certificate));
    BC_REQUIRE(context.certificates().empty());
}

BC_AUTO_TEST_CASE(tls_context__add_anchors__two__accumulated)
{
    tls::context context{};
    BC_REQUIRE(context.add_anchors(certificate_of(key1)));
    BC_REQUIRE(context.add_anchors(certificate_of(key2)));
    BC_REQUIRE_EQUAL(context.anchors().size(), 2u);
}

BC_AUTO_TEST_CASE(tls_context__set_verify__require_without_request__neither)
{
    tls::context context{};
    context.set_verify(false, true);
    BC_REQUIRE(!context.request());
    BC_REQUIRE(!context.require());

    context.set_verify(true, true);
    BC_REQUIRE(context.request());
    BC_REQUIRE(context.require());
}

BC_AUTO_TEST_CASE(tls_context__set_time__fixed__fixed)
{
    tls::context context{};
    context.set_time(42);
    BC_REQUIRE_EQUAL(context.time(), 42u);
}

BC_AUTO_TEST_CASE(tls_library__always__internal_tls)
{
    BC_REQUIRE_EQUAL(tls_library(), "internal tls 1.3");
}

BC_AUTO_TEST_SUITE_END()
