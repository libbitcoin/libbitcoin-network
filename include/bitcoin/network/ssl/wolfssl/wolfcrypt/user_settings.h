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
#ifndef LIBBITCOIN_NETWORK_SSL_WOLFSSL_WOLFCRYPT_USER_SETINGS_H
#define LIBBITCOIN_NETWORK_SSL_WOLFSSL_WOLFCRYPT_USER_SETINGS_H

/* This file is imported into all wolfssl sources when WOLFSSL_USER_SETTINGS */
/* is defined, which is done at project scope. This is also imported by boost */
/* asio via options.h import when BOOST_ASIO_USE_WOLFSSL is defined. */
/* The /ssl/openssl/ directory is provided for direct import by boost when */
/* #include <boost/asio/ssl.hpp> is specified, forwarding to wolfssl headers. */

/* This build has no dependency on any libbitcoin sources. */
/* #include <bitcoin/network/define.hpp> */
#if defined(_MSC_VER)
    /* C99 variable-length arrays (VLAs) are not supported by MSVC. */
    #define WOLFSSL_SP_NO_DYN_STACK

    /* Avoid conflict with min/max compatibility macros. */
    #define WOLFSSL_HAVE_MIN
    #define WOLFSSL_HAVE_MAX
#endif

/* Documentation for the options below. */
/* wolfssl.com/documentation/manuals/wolfssl/chapter02.html */

/* In a library build, "HAVE_" symbols are set on the command line. But since */
/* this is embedded they are set here just as with "NO_", "WC_" and "WOLFSSL_". */

/* Used with BOOST_ASIO_USE_WOLFSSL to optimize boost integration. */
/* wolfssl.com/wolfssl-support-asio-boost-asio-c-libraries */
#define WOLFSSL_ASIO

/* Suppress warnings on unnecessary file inclusions. */
#define WOLFSSL_IGNORE_FILE_WARN
#define WOLFSSL_VERBOSE_ERRORS

/* wolfssl.com/documentation/manuals/wolfssl/chapter05.html */
/* Requires that send and receive data copy functions be defined. */
/* #define WOLFSSL_USER_IO */

/* No reason to define this as the library does not use sockets (test only). */
/* #define WOLFSSL_NO_SOCK */

/* Required by boost for certificate management via filesystem. */
/* #define NO_FILESYSTEM */
#define WOLFSSL_PEM_TO_DER
#define WOLFSSL_CERT_GEN
#define WOLFSSL_DER_LOAD
#define WOLFSSL_KEY_GEN
#define WOLFSSL_SHA512
#define WOLFSSL_SHA384

/* TLS is required, not just cryptographic functions. */
#define WOLFSSL_TLS13

/* At least one encryption method is required. */
/* ECC is needed for Curve25519-based key exchange in modern TLS. */
#define HAVE_SUPPORTED_CURVES
#define HAVE_TLS_EXTENSIONS
#define HAVE_CURVE25519
#define HAVE_POLY1305
#define HAVE_ED25519
#define HAVE_CHACHA
#define HAVE_AESGCM
#define HAVE_HKDF
#define HAVE_HMAC
#define HAVE_ECC

/* Callback requires at least one union element to be defined. */
#define WOLF_CRYPTO_CB
/* This removes default RNG fallback (must set a callback RNG). */
/* #define WC_NO_HASHDRBG */
/* On Windows OS RNG APIs are used, these on others. */
/* #define NO_DEV_RANDOM */
/* #define NO_DEV_URANDOM */

/* These are openssl settings that affect wolfssl and/or boost asio. */
#define OPENSSL_VERSION_NUMBER 0x10101000L
#define OPENSSL_NO_ENGINE
#define OPENSSL_NO_SSL3
#define OPENSSL_NO_SSL2
#define OPENSSL_EXTRA

/* Side-channel protection is not required. */
/* #define WOLFSSL_HARDEN_TLS 128 */
#define NO_ECC_TIMING_RESISTANT
#define NO_TFM_TIMING_RESISTANT
#define WC_NO_HARDEN

/* Remove unused or undesired components. */
#define WOLFSSL_NO_CLIENT_AUTH
#define NO_SESSION_CACHE
#define NO_PWDBASED
#define NO_OLD_TLS
#define NO_OCSP
#define NO_DES3
#define NO_PSK
#define NO_SHA
#define NO_DSA
#define NO_RSA
#define NO_MD4
#define NO_MD5
#define NO_RC4
#define NO_DH

#ifdef _MSC_VER
    /* Warnings emitted due to missing define in vc++. */
    #define WC_MAYBE_UNUSED __pragma(warning(suppress:4505))

    /* Build break in test_ossl_bio.c due to missing define in vc++. */
    #ifndef STDERR_FILENO
    #define STDERR_FILENO _fileno(stderr)
    #endif
#endif

// Not setting this results in generic codes return from ssl via boost, but
// otherwise some failed calls return success due to lack of error queue being
// populated after a failed API call.
////#define WOLFSSL_HAVE_ERROR_QUEUE

/// Debugging information.
#if !defined(NDEBUG)
    // This will crash msvcrt on initialization if locale has been set.
    // Work around using logging callback and avoid locale-dependent writes.
    ////#define DEBUG_WOLFSSL

    // These require DEBUG_WOLFSSL.
    ////#ifndef WOLFSSL_LOGGINGENABLED_DEFAULT
    ////#define WOLFSSL_LOGGINGENABLED_DEFAULT 1
    ////#endif
    ////#ifndef WOLFSSL_CERT_LOG_ENABLED_DEFAULT
    ////#define WOLFSSL_CERT_LOG_ENABLED_DEFAULT 1
    ////#endif

    // These don't compile in vc++ (undefined pthread_mutex_lock).
    ////#define WOLFSSL_TRACK_MEMORY
    ////#define WOLFSSL_DEBUG_MEMORY
#endif

/// WolfSSL tests.
#define NO_MAIN_DRIVER
#define NO_TESTSUITE_MAIN_DRIVER
#define CERT_WRITE_TEMP_DIR "./"
#define DEBUG_SUITE_TESTS

#endif
