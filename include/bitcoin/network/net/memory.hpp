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
#ifndef LIBBITCOIN_NETWORK_NET_MEMORY_HPP
#define LIBBITCOIN_NETWORK_NET_MEMORY_HPP

#include <memory>
#include <shared_mutex>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

class BCT_API memory
{
public:
    DELETE_COPY_MOVE_DESTRUCT(memory);

    memory() NOEXCEPT;
    memory(arena* arena) NOEXCEPT;

    arena* get_arena() NOEXCEPT;
    retainer::ptr get_retainer() NOEXCEPT;


private:
    arena* arena_;
    std::shared_mutex remap_mutex_{};
};

} // namespace network
} // namespace libbitcoin

#endif
