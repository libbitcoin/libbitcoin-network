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
#ifndef LIBBITCOIN_NETWORK_BOOST_HPP
#define LIBBITCOIN_NETWORK_BOOST_HPP

#include <bitcoin/network/preprocessor.hpp>

// Must pull in any base boost configuration before including boost.
#include <bitcoin/system.hpp>

#include <boost/bimap.hpp>
#include <boost/bimap/set_of.hpp>
#include <boost/bimap/multiset_of.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/system/error_code.hpp>

#if defined(HAVE_SSL)
    #define BOOST_ASIO_USE_WOLFSSL
#endif

/// SSL is always defined, must be externally linked if !HAVE_SSL.
#include <boost/asio/ssl.hpp>

#endif
