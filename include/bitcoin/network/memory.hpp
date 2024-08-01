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
#ifndef LIBBITCOIN_NETWORK_MEMORY_HPP
#define LIBBITCOIN_NETWORK_MEMORY_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Tracked memory allocation interface.
class BCT_API memory
{
public:
    /// Get memory arena.
    virtual arena* get_arena() NOEXCEPT = 0;

    /// Get memory retainer.
    virtual retainer::ptr get_retainer() NOEXCEPT = 0;
};

/// Default tracked memory implementation (untracked).
class BCT_API default_memory final
  : public memory
{
public:
    DELETE_COPY_MOVE_DESTRUCT(default_memory);

    default_memory() NOEXCEPT;

    /// Get memory arena (system default).
    arena* get_arena() NOEXCEPT override;

    /// Get memory retainer (empty pointer).
    retainer::ptr get_retainer() NOEXCEPT override;
};

} // namespace network
} // namespace libbitcoin

#endif
