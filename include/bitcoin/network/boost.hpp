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
#ifndef LIBBITCOIN_NETWORK_BOOST_HPP
#define LIBBITCOIN_NETWORK_BOOST_HPP

#include <bitcoin/network/preprocessor.hpp>

// Must pull in any base boost configuration before including boost.
#include <bitcoin/system.hpp>

#include <boost/system/error_code.hpp>

/// SSL is always defined, must be externally linked if !HAVE_SSL.
#if defined(HAVE_SSL)
    #include <bitcoin/network/ssl/openssl/ssl.h>
#endif

/// boost/asio/ssl.hpp less the deprecated rfc2818_verification.
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/context_base.hpp>
#include <boost/asio/ssl/error.hpp>
#include <boost/asio/ssl/host_name_verification.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/asio/ssl/stream_base.hpp>
#include <boost/asio/ssl/verify_context.hpp>
#include <boost/asio/ssl/verify_mode.hpp>

#endif
