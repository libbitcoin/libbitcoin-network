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
#ifndef LIBBITCOIN_NETWORK_SSL_OPENSSL_OPENSSL_H
#define LIBBITCOIN_NETWORK_SSL_OPENSSL_OPENSSL_H

/* This directory is defined by libbitcoin to provide the expected path for */
/* boost::asio to #include the contained headers. Each header forwards to: */
/* #include <wolfssl/openssl/*.h> */

/* The inclusion of the above openssl headers and invocation of contained */
/* functions by boost requires that these all exist in the global namespace. */
/* Wolfssl consistently prefixes these C names, however they remain global */
/* and must be exposed to all dependent projects. The larger pollution is */
/* dependent headers that these headers then include, which export a very */
/* large number of symbols. By forwarding all of the functions invoked by */
/* boost these other headers and their symbols could be isolated to the */
/* compilation units. The forwarding is large however, up to 100 functions. */
/* Furthermore wolfssl tests reach directly into these secondary includes. */
/* So to retain the test coverage would require accessible headers. */

#endif
