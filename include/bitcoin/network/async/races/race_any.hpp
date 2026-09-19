/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_ANY_HPP
#define LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_ANY_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/async/handlers.hpp>

namespace libbitcoin {
namespace network {

/// Thread safe.
/// race_any invokes complete(args) provided at construct, with args from the
/// first call to finish(args), otherwise with error::operation_failed upon
/// release of the last reference.
template <typename... Args>
class race_any final
{
public:
    typedef std::shared_ptr<race_any> ptr;
    typedef std::function<void(Args...)> handler;

    DELETE_COPY_MOVE(race_any);

    race_any(handler&& complete) NOEXCEPT;
    ~race_any() NOEXCEPT;

    /// True implies winning finisher, there is at most one.
    bool finish(const Args&... args) NOEXCEPT;

private:
    // This is thread safe.
    std::atomic_bool finished_{};

    // This is protected by finished_.
    handler complete_;
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/races/race_any.ipp>

#endif
