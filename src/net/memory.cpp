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
#include <bitcoin/network/net/memory.hpp>

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

memory::memory() NOEXCEPT
  : memory(default_arena::get())
{
}

memory::memory(arena* arena) NOEXCEPT
  : arena_(arena)
{
}

arena* memory::get_arena() NOEXCEPT
{
    return arena_;
}

retainer::ptr memory::get_retainer() NOEXCEPT
{
    // Takes a shared lock on remap_mutex_ until destruct, blocking remap.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return std::make_unique<retainer>(remap_mutex_);
    BC_POP_WARNING()
}

} // namespace network
} // namespace libbitcoin
