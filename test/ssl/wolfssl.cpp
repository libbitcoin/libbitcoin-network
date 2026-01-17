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
#include "../test.hpp"

extern "C" {
#include "tests/unit.h"
#include "wolfcrypt/test/test.h"
}

// NO_MAIN_DRIVER and NO_TESTSUITE_MAIN_DRIVER must be set.
// These can be set, but are disabled in `unit_tests()` given current config.
// wolfSSL_Debugging_ON()
// wolfSSL_SetMemFailCount(memFailCount)
// wc_SetSeed_Cb(WC_GENERATE_SEED_DEFAULT)
// wc_InitNetRandom(wnrConfig, NULL, 5000)
// wc_RunCast_fips(setting)
// wc_RunAllCast_fips()

BOOST_FIXTURE_TEST_SUITE(wolfssl_tests, test::directory_setup_fixture)

BOOST_AUTO_TEST_CASE(wolfssl__w64wrapper__always__success)
{
#if defined(WOLFSSL_W64_WRAPPER)
    BOOST_REQUIRE(is_zero(w64wrapper_test()));
#endif
}

BOOST_AUTO_TEST_CASE(wolfssl__srp__always__success)
{
#if defined(WOLFCRYPT_HAVE_SRP) && defined(WOLFSSL_SHA512)
    BOOST_REQUIRE_NO_THROW(SrpTest());
#endif
}

BOOST_AUTO_TEST_CASE(wolfssl__quic__always__success)
{
#if defined(WOLFSSL_QUIC)
    BOOST_REQUIRE(is_zero(QuicTest()));
#endif
}

BOOST_AUTO_TEST_CASE(wolfssl__wolfcrypt__always__success)
{
#if !defined(NO_CRYPT_TEST)

    // requires:
    // /vectors/certs/ecc-key.der
    // /vectors/certs/ca-ecc384-key.der
    // /vectors/certs/ca-ecc384-cert.pem
    // cert paths are wired in "test.c" as:
    // CERT_PREFIX "certs" CERT_PATH_SEP
    // and CERT_PREFIX is defined as absolute in the project build.
    // By default CERT_PREFIX is "./"
    // CERT_WRITE_TEMP_DIR is defined as absolute in the project build.

    // TODO: use fixture to set/reset working directory for base cert read.
    // TODO: then restore it when the method completes.
    // std::filesystem::current_path();

    func_args arguments{};
    wolfCrypt_Init();
    BOOST_REQUIRE(is_zero(wolfcrypt_test(&arguments)));
    BOOST_REQUIRE(is_zero(arguments.return_code));
    wolfCrypt_Cleanup();

#endif
}

BOOST_AUTO_TEST_CASE(wolfssl__suite__always__success)
{
#if !defined(NO_WOLFSSL_CIPHER_SUITE_TEST) && \
    !defined(NO_WOLFSSL_CLIENT) && \
    !defined(NO_WOLFSSL_SERVER) && \
    !defined(NO_TLS) && \
    !defined(SINGLE_THREADED) && \
     defined(WOLFSSL_PEM_TO_DER)

    // requires:
    // /vectors/certs/*.pem
    // /vectors/certs/test/*.pem
    // /vectors/test.conf
    // cert paths are configured in "test.conf" only as: "./certs"
    // and the prefix is the relative working directory.

    // TODO: use fixture to set/reset working directory for base cert read.
    // TODO: then restore it when the method completes.
    // std::filesystem::current_path();

    // "test.conf" must have only '\n' line termination (not '\r\n').
    // Otherwise the file will be read as a single line and bypass all tests.
    // Defaults to 'const char* fname = "tests/test.conf";'.
    const char* configuration = CERT_PREFIX "tests/test.conf";
    const char* argv[]{ "testsuite", configuration };
    constexpr int argc = 2;

    // TODO: failing on connect.
    const auto result = SuiteTest(argc, const_cast<char**>(argv));
    BOOST_REQUIRE(!is_zero(result));
#endif
}

BOOST_AUTO_TEST_SUITE_END()
