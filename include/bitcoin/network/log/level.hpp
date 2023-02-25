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
#ifndef LIBBITCOIN_NETWORK_LOG_LEVEL_HPP
#define LIBBITCOIN_NETWORK_LOG_LEVEL_HPP

#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace level_t {

// Use level_t namespace to prevent pollution of network namesapce.
// Could use class enum, but we want simple conversion to uint8_t.
enum level : uint8_t
{
    quit,       // Quitting
    objects,    // Objects
    news,       // News
    session,    // Sessions/connect/accept
    protocol,   // Protocols
    proxy,      // proXy/socket/channel
    remote,     // Remote behavior
    fault,      // Fault
    reserved    // Unused by network lib.
};

#if defined(HAVE_EVENTS)
    #define FIRE_ONLY(name) name
    #define FIRE(type) fire(type)
    #define COUNT(type, count) fire(type, count)
#else
    #define FIRE_ONLY(name)
    #define FIRE(type)
    #define COUNT(type, count)
#endif

// LOG_ONLY() is insufficient for individual disablement.
#if defined(HAVE_LOGGING)
    #define LOG_ONLY(name) name
    #define LOG(level, message) \
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT) \
        log().write(level_t::level) << message << std::endl; \
        BC_POP_WARNING()
#else
    #define LOG_ONLY(name)
    #define LOG(level, message)
#endif

#if defined(HAVE_LOGQ)
    #define LOGQ(message) LOG(quit, message)
#else
    #define LOGQ(message)
#endif

#if defined(HAVE_LOGN)
    #define LOGN(message) LOG(news, message)
#else
    #define LOGN(message)
#endif

#if defined(HAVE_LOGX)
    #define LOGX(message) LOG(proxy, message)
#else
    #define LOGX(message)
#endif

#if defined(HAVE_LOGS)
    #define LOGS(message) LOG(session, message)
#else
    #define LOGS(message)
#endif

#if defined(HAVE_LOGP)
    #define LOGP(message) LOG(protocol, message)
#else
    #define LOGP(message)
#endif

#if defined(HAVE_LOGF)
    #define LOGF(message) LOG(fault, message)
#else
    #define LOGF(message)
#endif

#if defined(HAVE_LOGR)
    #define LOGR(message) LOG(remote, message)
#else
    #define LOGR(message)
#endif

#if defined(HAVE_LOGO)
    #define LOGO(message) LOG(objects, message)
#else
    #define LOGO(message)
#endif


} // namespace event_t
} // namespace network
} // namespace libbitcoin

#endif
