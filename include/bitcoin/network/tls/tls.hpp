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
#ifndef LIBBITCOIN_NETWORK_TLS_TLS_HPP
#define LIBBITCOIN_NETWORK_TLS_TLS_HPP

// TLS 1.3 server (rfc8446), the only protocol version supported.
// ----------------------------------------------------------------------------
// Cipher suites:   TLS_AES_128_GCM_SHA256, TLS_CHACHA20_POLY1305_SHA256.
// Key exchange:    X25519 (HelloRetryRequest for a client share of another
//                  group).
// Server signing:  ECDSA P-256 with SHA-256 (constant time, rfc6979 nonce).
// Client verify:   ECDSA P-256 with SHA-256, P-384 with SHA-384.
// Trust:           configured anchors only (no system store); a self-signed
//                  anchor is allowed.
// Not supported:   TLS 1.2 and earlier, RSA, Ed25519, resumption, pre-shared
//                  keys, 0-RTT, OCSP and CRL.

#include <bitcoin/network/tls/codec.hpp>
#include <bitcoin/network/tls/constants.hpp>
#include <bitcoin/network/tls/context.hpp>
#include <bitcoin/network/tls/record.hpp>
#include <bitcoin/network/tls/schedule.hpp>
#include <bitcoin/network/tls/server.hpp>

#endif
