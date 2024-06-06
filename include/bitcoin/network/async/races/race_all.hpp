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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_ALL_HPP
#define LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_ALL_HPP

#include <functional>
#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/async/handlers.hpp>

namespace libbitcoin {
namespace network {

template <typename... Args>
class race_all final
{
public:
    typedef std::shared_ptr<race_all> ptr;
    typedef std::function<void(Args...)> handler;

    DEFAULT_COPY_MOVE(race_all);

    race_all(handler&& complete) NOEXCEPT;
    ~race_all() NOEXCEPT;

private:
    handler complete_;
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/races/race_all.ipp>

#endif
