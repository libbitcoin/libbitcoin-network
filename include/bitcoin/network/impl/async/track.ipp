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
#ifndef LIBBITCOIN_NETWORK_ASYNC_TRACK_IPP
#define LIBBITCOIN_NETWORK_ASYNC_TRACK_IPP

#include <atomic>
#include <cstddef>
#include <typeinfo>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

// Log name.
#define LOG_SYSTEM "system"

namespace libbitcoin {
namespace network {

template <class Shared>
std::atomic<size_t> track<Shared>::instances_(0);

template <class Shared>
track<Shared>::track()
{
#ifndef NDEBUG
    ////LOG_DEBUG(LOG_SYSTEM) << typeid(Shared).name()
    ////    << "(" << ++instances_ << ")" << std::endl;
#endif
}

template <class Shared>
track<Shared>::~track() NOEXCEPT
{
#ifndef NDEBUG
    ////LOG_DEBUG(LOG_SYSTEM) << "~" << typeid(Shared).name()
    ////    << "(" << --instances_ << ")" << std::endl;
#endif
}

} // namespace network
} // namespace libbitcoin

#endif
