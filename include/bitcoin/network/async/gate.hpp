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
#ifndef LIBBITCOIN_NETWORK_ASYNC_GATE_HPP
#define LIBBITCOIN_NETWORK_ASYNC_GATE_HPP

#include <memory>
#include <tuple>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe.
/// Gate is a bind that invokes handler with the first set of arguments
/// but only after a preconfigured number of invocations. This assists in
/// Synchronizing the results of a set of racing synchronous operations.
template <size_t Knocks, typename Handler, typename... Args>
class gate_first final
{
public:
    DELETE_COPY_MOVE(gate_first);

    /// Create/destroy an unlocked gate.
    gate_first() NOEXCEPT;
    ~gate_first() NOEXCEPT;

    /// True if the gate is locked.
    bool locked() const NOEXCEPT;

    /// False implies invalid usage.
    bool lock(Handler&& handler) NOEXCEPT;

    /// False implies invalid usage.
    bool knock(Args&&... args) NOEXCEPT;

private:
    using sequence = std::index_sequence_for<Args...>;
    using arguments = std::tuple<std::decay_t<Args>...>;

    static_assert(!is_zero(Knocks));
    constexpr bool first_knock() const NOEXCEPT;

    template<size_t... Index>
    void invoker(const Handler& handler, const arguments& args,
        std::index_sequence<Index...>) NOEXCEPT;

    bool last_knock() NOEXCEPT;
    bool invoke() NOEXCEPT;

    size_t count_{};
    arguments args_{};
    std::shared_ptr<Handler> handler_{};
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/gate.ipp>

#endif
