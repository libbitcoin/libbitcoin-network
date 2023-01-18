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
#include <typeinfo>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/logger.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

template <class Shared, bool Track>
std::atomic<size_t> track<Shared, Track>::instances_(0);

template <class Shared, bool Track>
track<Shared, Track>::track(const logger& log) NOEXCEPT
  : log_(log)
{
    if constexpr (Track && bc::build_checked)
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        log_.write() << typeid(Shared).name()
            << "(" << ++instances_ << ")" << std::endl;
        BC_POP_WARNING()
    }
}

template <class Shared, bool Track>
track<Shared, Track>::~track() NOEXCEPT
{
    if constexpr (Track && bc::build_checked)
    {
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        log_.write() << typeid(Shared).name()
            << "(" << --instances_ << ")~" << std::endl;
        BC_POP_WARNING()
    }
}

template <class Shared, bool Track>
const logger& track<Shared, Track>::get_log() const NOEXCEPT
{
    return log_;
}

} // namespace network
} // namespace libbitcoin

#endif
