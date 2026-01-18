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

// If not defined in build config match wolfcrypt/test/test.c.
#ifndef CERT_PREFIX
#define CERT_PREFIX "./"
#endif

// These can be set, but are disabled in `unit_tests()` given current config.
// wolfSSL_Debugging_ON()
// wolfSSL_SetMemFailCount(memFailCount)
// wc_SetSeed_Cb(WC_GENERATE_SEED_DEFAULT)
// wc_InitNetRandom(wnrConfig, NULL, 5000)
// wc_RunCast_fips(setting)
// wc_RunAllCast_fips()

BOOST_FIXTURE_TEST_SUITE(wolfssl_tests, test::current_directory_setup_fixture)

#if defined(WOLFSSL_W64_WRAPPER)
BOOST_AUTO_TEST_CASE(wolfssl__w64wrapper__always__success)
{
    BOOST_REQUIRE(is_zero(w64wrapper_test()));
}
#endif

#if defined(WOLFCRYPT_HAVE_SRP) && defined(WOLFSSL_SHA512)
BOOST_AUTO_TEST_CASE(wolfssl__srp__always__success)
{
    BOOST_REQUIRE_NO_THROW(SrpTest());
}
#endif

#if defined(WOLFSSL_QUIC)
BOOST_AUTO_TEST_CASE(wolfssl__quic__always__success)
{
    BOOST_REQUIRE(is_zero(QuicTest()));
}
#endif

// Disabled until setting CERT_PREFIX is worked out.
#if defined(HAVE_MSC)

#if !defined(NO_CRYPT_TEST)
BOOST_AUTO_TEST_CASE(wolfssl__wolfcrypt__always__success)
{
    // requires:
    // /vectors/certs/ecc-key.der
    // /vectors/certs/ca-ecc384-key.der
    // /vectors/certs/ca-ecc384-cert.pem
    // cert paths are wired in "test.c" as:

    // By default CERT_PREFIX is "./" (relative),
    // and combined as: CERT_PREFIX "certs" CERT_PATH_SEP
    // but CERT_PREFIX is defined as absolute in the project build.

    // By default CERT_WRITE_TEMP_DIR is CERT_PREFIX, but this is absolute, so
    // CERT_WRITE_TEMP_DIR is predefined as relative ("./") in user_settings.h
    // Working directory is then controlled by current_directory_setup_fixture.

    func_args arguments{};
    wolfCrypt_Init();
    BOOST_REQUIRE(is_zero(wolfcrypt_test(&arguments)));
    BOOST_REQUIRE(is_zero(arguments.return_code));
    wolfCrypt_Cleanup();
}
#endif

#if !defined(NO_WOLFSSL_CIPHER_SUITE_TEST) && \
    !defined(NO_WOLFSSL_CLIENT) && \
    !defined(NO_WOLFSSL_SERVER) && \
    !defined(NO_TLS) && \
    !defined(SINGLE_THREADED) && \
     defined(WOLFSSL_PEM_TO_DER)
BOOST_AUTO_TEST_CASE(wolfssl__suite__always__success)
{
    // "test.conf" must have only '\n' line termination (not '\r\n').
    // Otherwise the file will be read as a single line and bypass all tests.
    // SuiteTest also bypasses any test for which the cert file is not found.

    // requires:
    // /vectors/certs/*.pem
    // /vectors/certs/test/*.pem
    // /vectors/tests/test.conf

    // cert paths are configured in "test.conf" only as: "./certs" (relative).
    // test.conf defaults to "tests/test.conf" (parameterizable). Since we need
    // to set the working directory for certs, we can use it for both.
    // Working directory is restored by current_directory_setup_fixture,

    code ec{};
    std::filesystem::current_path(CERT_PREFIX, ec);
    BOOST_REQUIRE(!ec);

    constexpr int argc{};
    const char* args[]{ "", nullptr };
    BC_PUSH_WARNING(NO_CONST_CAST)
    BC_PUSH_WARNING(NO_CONST_CAST_REQUIRED)
    auto argv = const_cast<char**>(args);
    BC_POP_WARNING()
    BC_POP_WARNING()
    BOOST_REQUIRE(is_zero(SuiteTest(argc, argv)));
}
#endif

#endif // HAVE_MSC

BOOST_AUTO_TEST_SUITE_END()
