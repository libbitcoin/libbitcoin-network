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
#ifndef LIBBITCOIN_NETWORK_WOLFSSL_USER_SETINGS_HPP
#define LIBBITCOIN_NETWORK_WOLFSSL_USER_SETINGS_HPP

////#include <bitcoin/network/define.hpp>

// Suppress warnings on unnecessary file inclusions.
#define WOLFSSL_IGNORE_FILE_WARN

// Prevent errors due to some define of min/max macros (not from windows.h).
#define WOLFSSL_HAVE_MIN
#define WOLFSSL_HAVE_MAX

// These compile but are not required.
#define NO_FILESYSTEM
#define WC_NO_HASHDRBG
#define NO_OLD_TLS
#define NO_AESGCM
#define NO_DES3
#define NO_OCSP
#define NO_PSK
#define NO_AES
#define NO_SHA

// TLS is required, not just cryptographic functions.
// These are not required to compile but features are required.
////#define WOLFCRYPT_ONLY
#define HAVE_TLS_EXTENSIONS
#define HAVE_FFDHE_2048
#define WOLFSSL_TLS13
#define HAVE_POLY1305
#define HAVE_CHACHA

// These require bigint.
#define OPENSSL_EXTRA
#define NO_PWDBASED
#define NO_DSA
#define NO_DH
#define NO_RSA
////#define NO_ASN

// TODO: implement HKDF template natively.
// TODO: use own HKDF, HMAC, SHA256 in callback.
// Callback requires at least one element of union to be defined.
#define WOLF_CRYPTO_CB
#define WOLFSSL_KEY_GEN
#define WC_NO_HARDEN
////#define WC_NO_RNG
////#define HAVE_HKDF
////#define NO_SHA256
////#define NO_HMAC

// At least one encryption method is required (but bigint is required).
#define HAVE_CURVE25519
#define HAVE_ECC
#define NO_ECC

// This is requried for bigint on vc++.
// C99 variable-length arrays (VLAs) are not supported by MSVC.
#if defined(_MSC_VER)
#define WOLFSSL_SP_NO_DYN_STACK
#endif

#endif
