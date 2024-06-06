/**
 * Copyright (c) 2011-2024 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_ALL_IPP
#define LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_ALL_IPP

#include <utility>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/async/handlers.hpp>

namespace libbitcoin {
namespace network {

template <typename... Args>
race_all<Args...>::
race_all(handler&& complete) NOEXCEPT
  : complete_(std::move(complete))
{
}

template <typename... Args>
race_all<Args...>::
~race_all() NOEXCEPT
{
    complete_(error::success);
}

} // namespace network
} // namespace libbitcoin

#endif
