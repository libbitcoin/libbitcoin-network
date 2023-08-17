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
#ifndef LIBBITCOIN_NETWORK_ASYNC_HANDLERS_HPP
#define LIBBITCOIN_NETWORK_ASYNC_HANDLERS_HPP

#include <functional>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

// Utility to convert a const reference instance to moveable.
template <typename Type>
Type move_copy(const Type& instance) NOEXCEPT
{
    auto copy = instance;
    return copy;
}

// Common handlers, used across types.
typedef std::function<bool(const code&)> notify_handler;
typedef std::function<void(const code&)> result_handler;
typedef std::function<void(const code&, size_t)> count_handler;

} // namespace network
} // namespace libbitcoin

#endif
