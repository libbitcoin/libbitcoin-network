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
#include <bitcoin/network/boost.hpp>

#include <cstdlib>
#include <bitcoin/network/define.hpp>

#if defined(BOOST_ASIO_USE_WOLFSSL)

// boost::asio::ssl initializes ssl via a singleton and does not uninitialize
// it at shutdown. This results from importing <boost/asio/ssl.hpp> and invokes
// ::SSL_library_init(), which in turn invokes wolfSSL_Init(). Because wolfssl
// reference counts the initialization, the cleanup call must be balanced in
// any process that includes network. Otherwise it leaks at least 136 bytes.
struct boost_openssl_cleanup
{
    DEFAULT_COPY_MOVE_DESTRUCT(boost_openssl_cleanup);

    // Invoked prior to main() to register cleanup on shutdown.
    boost_openssl_cleanup() NOEXCEPT
    {
        std::atexit([]() NOEXCEPT { wolfSSL_Cleanup(); });
    }
} boost_openssl_cleanup;

#endif
