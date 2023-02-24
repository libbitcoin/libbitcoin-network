/**
 * Copyright (c) 2011-2022 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_ASYNC_REPORTER_HPP
#define LIBBITCOIN_NETWORK_ASYNC_REPORTER_HPP

#include <iostream>
#include <bitcoin/network/async/logger.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

class BCT_API reporter
{
protected:
    reporter(const logger& log) NOEXCEPT;

public:
    const logger& log() const NOEXCEPT;
    const void fire(uint8_t identifier, size_t count=zero) const NOEXCEPT;

private:
    // This is thread safe.
    const logger& log_;
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

#if defined(HAVE_LOGGING)
    #define LOG_ONLY(name) name
    #define LOG(message) \
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT) \
        log().write() << message << std::endl; \
        BC_POP_WARNING()
#else
    #define LOG_ONLY(name)
    #define LOG(message)
#endif

} // namespace network
} // namespace libbitcoin

#endif
