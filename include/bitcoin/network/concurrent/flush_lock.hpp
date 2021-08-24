/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_CONCURRENT_FLUSH_LOCK_HPP
#define LIBBITCOIN_NETWORK_CONCURRENT_FLUSH_LOCK_HPP

#include <boost/filesystem.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/concurrent/file_lock.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// This class is not thread safe, and does not throw.
class BCT_API flush_lock
  : file_lock
{
public:
    flush_lock(const boost::filesystem::path& file) noexcept;

    /// False if file exists.
    bool try_lock() const noexcept;

    /// False if file exists or fails to create.
    bool lock() noexcept;

    /// False if file does not exist or fails to delete.
    bool unlock() noexcept;
};

} // namespace network
} // namespace libbitcoin

#endif
