/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_DEFINE_HPP
#define LIBBITCOIN_NETWORK_DEFINE_HPP

#include <bitcoin/system.hpp>

// We use the generic helper definitions in libbitcoin to define BCT_API
// and BCT_INTERNAL. BCT_API is used for the public API symbols. It either DLL
// imports or DLL exports (or does nothing for static build) BCT_INTERNAL is
// used for non-api symbols.

#if defined BCT_STATIC
    #define BCT_API
    #define BCT_INTERNAL
#elif defined BCT_DLL
    #define BCT_API      BC_HELPER_DLL_EXPORT
    #define BCT_INTERNAL BC_HELPER_DLL_LOCAL
#else
    #define BCT_API      BC_HELPER_DLL_IMPORT
    #define BCT_INTERNAL BC_HELPER_DLL_LOCAL
#endif

/// WITH_ indicates build symbol.
/// ---------------------------------------------------------------------------

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

#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

#define BIND_SHARED(method, args) \
    std::bind(std::forward<Method>(method), shared_from_base<Derived>(), \
        std::forward<Args>(args)...)
#define BIND_THIS(method, args) \
    std::bind(std::forward<Method>(method), static_cast<Derived*>(this), \
    std::forward<Args>(args)...)
#define BIND(method, ...) \
    bind<CLASS>(&CLASS::method, __VA_ARGS__)
#define POST(method, ...) \
    post<CLASS>(&CLASS::method, __VA_ARGS__)

} // namespace network
} // namespace libbitcoin

// define.hpp is the common include for /network (except p2p and settings).
// All non-network headers include define.hpp.
// Network inclusions are chained as follows.

// version        : <generated>
// boost          : version
// error          : boost
// define         : error

// Other directory common includes are not internally chained.
// Each header includes only its required common headers.

// /async         : define
// /messages      : define
// /log           : define /async
// /config        : define /messages /async
// /net           : define settings /config /log
// /sessions      : define settings /net [forward: p2p]
// /protocols     : define settings /sessions

// Root directory singletons.

// settings       : define /messages /config
// p2p            : define settings /sessions

#endif
