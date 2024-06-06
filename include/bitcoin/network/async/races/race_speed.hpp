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
#ifndef LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_SPEED_HPP
#define LIBBITCOIN_NETWORK_ASYNC_RACES_RACE_SPEED_HPP

#include <memory>
#include <tuple>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe.
/// Used in connector to race between timer (connection timeout) and connect.
/// race_speed<Size> invokes complete(args) provided at start(complete), with
/// args from first call to finish(args), upon the last expected invocation of
/// finish(...) based on the templatized Size number of expected calls.
template <size_t Size, typename... Args>
class race_speed final
{
public:
    typedef std::shared_ptr<race_speed> ptr;
    typedef std::function<void(Args...)> handler;

    /// A stopped_ member is sufficient for a race_speed of one.
    static_assert(Size > one);

    DELETE_COPY_MOVE(race_speed);

    race_speed() NOEXCEPT;
    ~race_speed() NOEXCEPT;

    /// True if the race_speed is running.
    inline bool running() const NOEXCEPT;

    /// False implies invalid usage.
    bool start(handler&& complete) NOEXCEPT;

    /// True implies winning finisher, there is always exactly one.
    bool finish(const Args&... args) NOEXCEPT;

private:
    template <size_t... Pack>
    using unpack = std::index_sequence<Pack...>;
    using packed = std::tuple<std::decay_t<Args>...>;
    using sequence = std::index_sequence_for<Args...>;

    template<size_t... Index>
    void invoker(const handler& complete, const packed& args,
        unpack<Index...>) NOEXCEPT;
    bool invoke() NOEXCEPT;
    bool set_final() NOEXCEPT;
    bool is_winner() const NOEXCEPT;

    // These are not thread safe.
    packed args_{};
    size_t runners_{};
    std::shared_ptr<handler> complete_{};
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/races/race_speed.ipp>

#endif
