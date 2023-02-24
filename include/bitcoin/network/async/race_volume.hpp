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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RACE_VOLUME_HPP
#define LIBBITCOIN_NETWORK_ASYNC_RACE_VOLUME_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe.
/// Race is a bind that invokes handler with the first set of arguments
/// but only after a preconfigured number of invocations. This assists in
/// synchronizing the results of a set of racing asynchronous operations.
template <error::error_t Success, error::error_t Fail>
class race_volume final
{
public:
    typedef std::shared_ptr<race_volume> ptr;
    typedef std::function<void(const code&)> handler;

    DELETE_COPY_MOVE(race_volume);

    race_volume(size_t size, size_t required) NOEXCEPT;
    ~race_volume() NOEXCEPT;

    /// True if the race_volume is running.
    inline bool running() const NOEXCEPT;

    /// False implies invalid usage.
    bool start(handler&& sufficient, handler&& complete) NOEXCEPT;

    /// Signal finisher and pass total count.
    bool finish(size_t count) NOEXCEPT;

private:
    bool invoke() NOEXCEPT;

    // These are thread safe.
    const size_t size_;
    const size_t required_;

    // These are not thread safe.
    size_t runners_{};
    std::shared_ptr<handler> sufficient_{};
    std::shared_ptr<handler> complete_{};
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/race_volume.ipp>

#endif
