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
#ifndef LIBBITCOIN_NETWORK_ASYNC_TRACK_HPP
#define LIBBITCOIN_NETWORK_ASYNC_TRACK_HPP

#include <atomic>
#include <cstddef>

namespace libbitcoin {
namespace network {

/// Thread safe, base class.
/// Class to log changes in the reference count of shared objects.
template <class Shared>
class track
{
protected:
    track();
    virtual ~track() noexcept;

private:
    // This is thread safe.
    static std::atomic<size_t> instances_;
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/track.ipp>

#endif