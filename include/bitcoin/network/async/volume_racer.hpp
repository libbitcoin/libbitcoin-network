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
#ifndef LIBBITCOIN_NETWORK_ASYNC_VOLUME_RACER_HPP
#define LIBBITCOIN_NETWORK_ASYNC_VOLUME_RACER_HPP

#include <memory>
#include <tuple>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe.
/// Race is a bind that invokes handler with the first set of arguments
/// but only after a preconfigured number of invocations. This assists in
/// synchronizing the results of a set of racing asynchronous operations.
template <size_t Size, typename... Args>
class volume_racer final
{
public:
    typedef std::function<void(Args...)> handler;

    /// A stopped_ member is sufficient for a volume_racer of one.
    static_assert(Size > one);

    DELETE_COPY_MOVE(volume_racer);

    volume_racer() NOEXCEPT;
    ~volume_racer() NOEXCEPT;

    /// True if the volume_racer is running.
    bool running() const NOEXCEPT;

    /// False implies invalid usage.
    bool start(handler&& complete) NOEXCEPT;

    /// False implies invalid usage.
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
    bool is_loser() NOEXCEPT;
    bool is_winner() const NOEXCEPT;

    packed args_{};
    size_t runners_{};
    std::shared_ptr<handler> complete_{};
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/volume_racer.ipp>

#endif
