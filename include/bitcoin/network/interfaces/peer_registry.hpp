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
#ifndef LIBBITCOIN_NETWORK_INTERFACES_PEER_REGISTRY_HPP
#define LIBBITCOIN_NETWORK_INTERFACES_PEER_REGISTRY_HPP

#include <array>
#include <tuple>
#include <utility>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/interfaces/peer_dispatch.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// The peer message set, derived from the dispatch interface. Command, index
/// and deserialization are obtained from one declaration, so a message cannot
/// be dispatchable yet unrecognized (or the reverse).
struct peer_registry
{
    using methods_t = decltype(peer_dispatch::methods);
    static constexpr auto size = std::tuple_size_v<methods_t>;

    /// Index of an unregistered command (one past the last valid index).
    static constexpr auto unknown = size;

    /// The message type registered at index.
    template <size_t Index>
    using cptr_t = std::tuple_element_t<zero,
        args_native_t<method_t<Index, methods_t>>>;
    template <size_t Index>
    using message_t = std::remove_const_t<typename cptr_t<Index>::element_type>;

    /// The wire command registered at index.
    template <size_t Index>
    static constexpr auto command = method_t<Index, methods_t>::name;

    using commands_t = std::array<std::string_view, size>;

private:
    using deserializer_t = any_t(*)(const system::data_chunk&, uint32_t, bool);
    using deserializers_t = std::array<deserializer_t, size>;

    /// Helpers are defined before use, as constant evaluation requires it.

    /// The witness parameter is detected, as only some messages carry it.
    template <size_t Index>
    static any_t deserialize(const system::data_chunk& data, uint32_t version,
        bool witness) NOEXCEPT
    {
        using message = message_t<Index>;
        typename message::cptr message_ptr{};

        if constexpr (requires { message::deserialize(version, data, witness); })
            message_ptr = message::deserialize(version, data, witness);
        else
            message_ptr = message::deserialize(version, data);

        return message_ptr ? any_t{ message_ptr } : any_t{};
    }

    template <size_t... Index>
    static constexpr commands_t make_commands(
        std::index_sequence<Index...>) NOEXCEPT
    {
        return { command<Index>... };
    }

    template <size_t... Index>
    static constexpr deserializers_t make_deserializers(
        std::index_sequence<Index...>) NOEXCEPT
    {
        return { &peer_registry::deserialize<Index>... };
    }

    template <size_t... Index>
    static constexpr size_t to_index(const std::string_view& command,
        std::index_sequence<Index...>) NOEXCEPT
    {
        auto found = unknown;
        const auto match = [&](size_t at, std::string_view name) NOEXCEPT
        {
            return name == command ? ((found = at), true) : false;
        };

        (void)(match(Index, peer_registry::command<Index>) || ...);
        return found;
    }

    template <class Message, size_t... Index>
    static constexpr size_t to_index_of(std::index_sequence<Index...>) NOEXCEPT
    {
        auto found = unknown;
        const auto match = [&](size_t at, bool same) NOEXCEPT
        {
            return same ? ((found = at), true) : false;
        };

        (void)(match(Index, std::is_same_v<Message, message_t<Index>>) || ...);
        return found;
    }

public:
    /// The registered commands, ordered by index.
    static const commands_t& commands() NOEXCEPT
    {
        static constexpr auto table = make_commands(
            std::make_index_sequence<size>{});

        return table;
    }

    /// Index of the command, unknown if not registered.
    static constexpr size_t index(const std::string_view& command) NOEXCEPT
    {
        return to_index(command, std::make_index_sequence<size>{});
    }

    /// Index of the message type, unknown if not registered.
    template <class Message>
    static constexpr size_t index_of() NOEXCEPT
    {
        return to_index_of<Message>(std::make_index_sequence<size>{});
    }

    /// Deserialize the payload as the message registered at index.
    /// Returns an empty any_t if the index is unknown or the parse fails.
    static any_t to_any(size_t index, const system::data_chunk& data,
        uint32_t version, bool witness) NOEXCEPT
    {
        static constexpr auto table = make_deserializers(
            std::make_index_sequence<size>{});

        return index < size ? table.at(index)(data, version, witness) :
            any_t{};
    }
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
