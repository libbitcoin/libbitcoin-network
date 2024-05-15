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
#ifndef LIBBITCOIN_NETWORK_LOG_LEVELS_HPP
#define LIBBITCOIN_NETWORK_LOG_LEVELS_HPP

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace levels {

// Could use class enum, but we want simple conversion to uint8_t.
enum : uint8_t
{
    application, // Unused by network lib
    news,        // News
    session,     // Sessions/connect/accept
    protocol,    // Protocols
    proxy,       // proXy/socket/channel
    remote,      // Remote behavior
    fault,       // Fault
    quitting,    // Quitting
    objects,     // Objects
    verbose      // Verbose
};

#if defined(HAVE_LOGGING)
    #define LOG_ONLY(name) name
    #define LOG(level_, message) \
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT) \
        log.write(network::levels::level_) << message << std::endl; \
        BC_POP_WARNING()
    #define LOG_LOG(name, level_) \
        log.write(network::levels::application) << name \
            << network::levels::level_ << std::endl;
#else
    #define LOG_ONLY(name)
    #define LOG(level, message)
    #define LOG_LOG(level_, message)
#endif

#if defined(HAVE_LOGO)
    constexpr auto objects_defined = true;
    #define LOGO(message) \
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT) \
        log_.write(network::levels::objects) << message << std::endl; \
        BC_POP_WARNING()
#else
    constexpr auto objects_defined = false;
    #define LOGO(message)
#endif

#if defined(HAVE_LOGA)
    constexpr auto application_defined = true;
    #define LOGA(message) LOG(application, message)
#else
    #define LOGA(message)
    constexpr auto application_defined = false;
#endif

#if defined(HAVE_LOGN)
    constexpr auto news_defined = true;
    #define LOGN(message) LOG(news, message)
#else
    #define LOGN(message)
    constexpr auto news_defined = false;
#endif

#if defined(HAVE_LOGS)
    constexpr auto session_defined = true;
    #define LOGS(message) LOG(session, message)
#else
    constexpr auto session_defined = false;
    #define LOGS(message)
#endif

#if defined(HAVE_LOGP)
    constexpr auto protocol_defined = true;
    #define LOGP(message) LOG(protocol, message)
#else
    constexpr auto protocol_defined = false;
    #define LOGP(message)
#endif

#if defined(HAVE_LOGX)
    constexpr auto proxy_defined = true;
    #define LOGX(message) LOG(proxy, message)
#else
    constexpr auto proxy_defined = false;
    #define LOGX(message)
#endif

#if defined(HAVE_LOGR)
    constexpr auto remote_defined = true;
    #define LOGR(message) LOG(remote, message)
#else
    constexpr auto remote_defined = false;
    #define LOGR(message)
#endif

#if defined(HAVE_LOGF)
    constexpr auto fault_defined = true;
    #define LOGF(message) LOG(fault, message)
#else
    constexpr auto fault_defined = false;
    #define LOGF(message)
#endif

#if defined(HAVE_LOGQ)
    constexpr auto quitting_defined = true;
    #define LOGQ(message) LOG(quitting, message)
#else
    constexpr auto quitting_defined = false;
    #define LOGQ(message)
#endif

#if defined(HAVE_LOGV)
    constexpr auto verbose_defined = true;
    #define LOGV(message) LOG(verbose, message)
#else
    #define LOGV(message)
    constexpr auto verbose_defined = false;
#endif

} // namespace levels
} // namespace network
} // namespace libbitcoin

#endif
