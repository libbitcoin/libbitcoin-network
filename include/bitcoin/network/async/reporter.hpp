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

private:
    // This is thread safe.
    const logger& log_;
};

#if defined(NDEBUG)
    #define LOG_ONLY(name)
    #define LOG(message)
#else
    #define LOG_ONLY(name) name
    #define LOG(message) log().write() << message << std::endl
#endif

} // namespace network
} // namespace libbitcoin

#endif
