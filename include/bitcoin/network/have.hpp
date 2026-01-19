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
#ifndef LIBBITCOIN_NETWORK_HAVE_HPP
#define LIBBITCOIN_NETWORK_HAVE_HPP

#include <bitcoin/network/version.hpp>

/// WITH_ indicates build symbol.
/// ---------------------------------------------------------------------------

// TODO: this conflicts with a scneario in which the developer wants to exclude
// the embedded wolfssl and link one externally, as this symbol must be defined
// in this and dependent projects so that boost can link the library with
// configuration. However that will also activate the embedded build.
/// This enables integral ssl support via embedded wolfssl.
/// If not defined then boost-compatible SSL must be externally linked.
/// The build config must always define either the internal ssl include path or
/// an external one, so that boost will locate the expected openssl headers.
#if defined(WOLFSSL_USER_SETTINGS)
    #define HAVE_SSL
#endif

/// TODO: Move to build configuration.
////#define WITH_EVENTS
#define WITH_LOGGING
#if !defined(NDEBUG)
    #define WITH_LOGO
#endif
#define WITH_LOGA
#define WITH_LOGN
#define WITH_LOGS
#define WITH_LOGP
#define WITH_LOGX
#define WITH_LOGR
#define WITH_LOGF
#define WITH_LOGQ
#define WITH_LOGV

////#if defined(WITH_EVENTS)
////    #define HAVE_EVENTS
////#endif
#if defined(WITH_LOGGING)
    #define HAVE_LOGGING

    /// Objects (shared object construct/destruct).
    #if defined(WITH_LOGO)
        #define HAVE_LOGO
    #endif

    #if defined(WITH_LOGA)
        #define HAVE_LOGA
    #endif

    /// News (general progression).
    #if defined(WITH_LOGN)
        #define HAVE_LOGN
    #endif

    /// Sessions.
    #if defined(WITH_LOGS)
        #define HAVE_LOGS
    #endif

    /// Protocols.
    #if defined(WITH_LOGP)
        #define HAVE_LOGP
    #endif

    /// ProXy.
    #if defined(WITH_LOGX)
        #define HAVE_LOGX
    #endif

    /// Remote (peer errors).
    #if defined(WITH_LOGR)
        #define HAVE_LOGR
    #endif

    /// Fault (own errors).
    #if defined(WITH_LOGF)
        #define HAVE_LOGF
    #endif

    /// Quitting connections (e.g. read/write/send abort).
    #if defined(WITH_LOGQ)
        #define HAVE_LOGQ
    #endif

    /// Verbose (ad-hoc debugging).
    #if defined(WITH_LOGV)
        #define HAVE_LOGV
    #endif
#endif

#endif
