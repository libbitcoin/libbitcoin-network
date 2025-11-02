/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_ASYNC_THREAD_HPP
#define LIBBITCOIN_NETWORK_ASYNC_THREAD_HPP

#include <memory>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
    
/// defaults to highest ("normal")
enum class memory_priority
{
    highest,
    high,
    medium,
    low,
    lowest
};

/// defaults to medium ("normal")
enum class processing_priority
{
    highest,
    high,
    medium,
    low,
    lowest
};

// Always at least 1 (guards against irrational API return).
BCT_API size_t cores() NOEXCEPT;

// Set memory priority for the current PROCESS.
BCT_API void set_memory_priority(memory_priority priority) NOEXCEPT;

// Set processing priority for the current THREAD.
BCT_API void set_processing_priority(processing_priority priority) NOEXCEPT;

} // namespace network
} // namespace libbitcoin

#endif
