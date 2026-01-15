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
#ifndef LIBBITCOIN_NETWORK_SSL_WOLFSSL_OPTIONS_H
#define LIBBITCOIN_NETWORK_SSL_WOLFSSL_OPTIONS_H

// This ensures boost sees all configuration, but probably not used.
#include <wolfssl/wolfcrypt/user_settings.h>

// Boost ASIO pulls in this file when BOOST_ASIO_USE_WOLFSSL is defined.
////#if defined(BOOST_ASIO_USE_WOLFSSL)
////    #include <wolfssl/options.h>
////#endif

#endif
