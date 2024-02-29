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
#define WITH_EVENTS
#define WITH_LOGGING
#if !defined(NDEBUG)
    #define WITH_LOGO
#endif
#define WITH_LOGN
#define WITH_LOGS
#define WITH_LOGP
#define WITH_LOGX
#define WITH_LOGW
#define WITH_LOGR
#define WITH_LOGF
#define WITH_LOGQ

#if defined(WITH_EVENTS)
    #define HAVE_EVENTS
#endif
#if defined(WITH_LOGGING)
    #define HAVE_LOGGING
    #if defined(WITH_LOGO)
        #define HAVE_LOGO
    #endif
    #if defined(WITH_LOGN)
        #define HAVE_LOGN
    #endif
    #if defined(WITH_LOGS)
        #define HAVE_LOGS
    #endif
    #if defined(WITH_LOGP)
        #define HAVE_LOGP
    #endif
    #if defined(WITH_LOGX)
        #define HAVE_LOGX
    #endif
    #if defined(WITH_LOGW)
        #define HAVE_LOGW
    #endif
    #if defined(WITH_LOGR)
        #define HAVE_LOGR
    #endif
    #if defined(WITH_LOGF)
        #define HAVE_LOGF
    #endif
    #if defined(WITH_LOGQ)
        #define HAVE_LOGQ
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

#define BIND1(method, p1) \
    bind<CLASS>(&CLASS::method, p1)
#define BIND2(method, p1, p2) \
    bind<CLASS>(&CLASS::method, p1, p2)
#define BIND3(method, p1, p2, p3) \
    bind<CLASS>(&CLASS::method, p1, p2, p3)
#define BIND4(method, p1, p2, p3, p4) \
    bind<CLASS>(&CLASS::method, p1, p2, p3, p4)
#define BIND5(method, p1, p2, p3, p4, p5) \
    bind<CLASS>(&CLASS::method, p1, p2, p3, p4, p5)
#define BIND6(method, p1, p2, p3, p4, p5, p6) \
    bind<CLASS>(&CLASS::method, p1, p2, p3, p4, p5, p6)
#define BIND7(method, p1, p2, p3, p4, p5, p6, p7) \
    bind<CLASS>(&CLASS::method, p1, p2, p3, p4, p5, p6, p7)

#define POST1(method, p1) \
    post<CLASS>(&CLASS::method, p1)
#define POST2(method, p1, p2) \
    post<CLASS>(&CLASS::method, p1, p2)
#define POST3(method, p1, p2, p3) \
    post<CLASS>(&CLASS::method, p1, p2, p3)

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
