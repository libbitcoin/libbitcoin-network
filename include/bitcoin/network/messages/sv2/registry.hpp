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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_SV2_REGISTRY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_SV2_REGISTRY_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/interfaces/sv2_dispatch.hpp>
#include <bitcoin/network/messages/sv2/sv2.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

/// The stratum v2 message codec, derived from the dispatch interface.
struct registry
{
    using methods_t = decltype(rpc::sv2_dispatch::methods);
    static constexpr auto size = std::tuple_size_v<methods_t>;

    /// Index of an unregistered message (one past the last valid index).
    static constexpr auto unknown = size;

    /// Handshake messages are registered first, this is their count.
    static constexpr auto handshakes = std::tuple_size_v<
        decltype(rpc::sv2_dispatch::handshake)>;

    /// The message type registered at index.
    template <size_t Index>
    using arguments_t = rpc::args_native_t<rpc::method_t<Index, methods_t>>;
    template <size_t Index>
    using cptr_t = std::tuple_element_t<zero, arguments_t<Index>>;
    template <size_t Index>
    using message_t = std::remove_const_t<typename cptr_t<Index>::element_type>;

    /// The method name registered at index.
    template <size_t Index>
    static constexpr auto command = rpc::method_t<Index, methods_t>::name;

    /// The message type identifier registered at index.
    template <size_t Index>
    static constexpr auto identifier = message_t<Index>::identifier;

    /// The message registered at index is addressed to a channel.
    template <size_t Index>
    static constexpr auto channel = message_t<Index>::channel;

    using commands_t = std::array<std::string_view, size>;
    using identifiers_t = std::array<uint8_t, size>;
    using channels_t = std::array<bool, size>;

    /// Byte counts, indexed by registered message.
    using counters_t = std::array<uint64_t, size>;

private:
    using span_t = std::span<const uint8_t>;
    using deserializer_t = rpc::any_t(*)(const span_t&);
    using deserializers_t = std::array<deserializer_t, size>;
    using serializer_t = system::chunk_ptr(*)(const rpc::any_t&);
    using serializers_t = std::array<serializer_t, size>;
    using framer_t = bool(*)(const rpc::any_t&, const system::data_slab&);
    using framers_t = std::array<framer_t, size>;

    template <size_t Index>
    static rpc::any_t deserialize(const span_t& data) NOEXCEPT
    {
        const auto ptr = message_t<Index>::deserialize(data);
        return ptr ? rpc::any_t{ ptr } : rpc::any_t{};
    }

    template <size_t Index>
    static system::chunk_ptr serialize(const rpc::any_t& message) NOEXCEPT
    {
        const auto ptr = message.get<const message_t<Index>>();
        return ptr ? sv2::serialize(*ptr) : system::chunk_ptr{};
    }

    template <size_t Index>
    static bool serialize_frame(const rpc::any_t& message,
        const system::data_slab& data) NOEXCEPT
    {
        const auto ptr = message.get<const message_t<Index>>();
        return ptr && sv2::serialize(*ptr, data);
    }

    template <size_t... Index>
    static constexpr deserializers_t make_deserializers(
        std::index_sequence<Index...>) NOEXCEPT
    {
        return { &registry::deserialize<Index>... };
    }

    template <size_t... Index>
    static constexpr serializers_t make_serializers(
        std::index_sequence<Index...>) NOEXCEPT
    {
        return { &registry::serialize<Index>... };
    }

    template <size_t... Index>
    static constexpr framers_t make_framers(
        std::index_sequence<Index...>) NOEXCEPT
    {
        return { &registry::serialize_frame<Index>... };
    }

    template <size_t... Index>
    static constexpr commands_t make_commands(
        std::index_sequence<Index...>) NOEXCEPT
    {
        return { command<Index>... };
    }

    template <size_t... Index>
    static constexpr identifiers_t make_identifiers(
        std::index_sequence<Index...>) NOEXCEPT
    {
        return { identifier<Index>... };
    }

    template <size_t... Index>
    static constexpr channels_t make_channels(
        std::index_sequence<Index...>) NOEXCEPT
    {
        return { channel<Index>... };
    }

    template <size_t... Index>
    static constexpr size_t to_index(uint8_t id,
        std::index_sequence<Index...>) NOEXCEPT
    {
        auto found = unknown;
        const auto match = [&](size_t at, uint8_t assigned) NOEXCEPT
        {
            const auto same = assigned == id;
            return same ? ((found = at), true) : false;
        };

        (match(Index, identifier<Index>) || ...);
        return found;
    }

    template <size_t... Index>
    static constexpr size_t to_index(const std::string_view& command,
        std::index_sequence<Index...>) NOEXCEPT
    {
        auto found = unknown;
        const auto match = [&](size_t at, const std::string_view& name) NOEXCEPT
        {
            const auto same = name == command;
            return same ? ((found = at), true) : false;
        };

        (match(Index, registry::command<Index>) || ...);
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

        (match(Index, std::is_same_v<Message, message_t<Index>>) || ...);
        return found;
    }

public:
    static const commands_t& commands() NOEXCEPT
    {
        static constexpr auto table = make_commands(
            std::make_index_sequence<size>{});

        return table;
    }

    static const identifiers_t& identifiers() NOEXCEPT
    {
        static constexpr auto table = make_identifiers(
            std::make_index_sequence<size>{});

        return table;
    }

    static const channels_t& channels() NOEXCEPT
    {
        static constexpr auto table = make_channels(
            std::make_index_sequence<size>{});

        return table;
    }

    static constexpr size_t index(const std::string_view& command) NOEXCEPT
    {
        return to_index(command, std::make_index_sequence<size>{});
    }

    static constexpr size_t index(uint8_t id) NOEXCEPT
    {
        return to_index(id, std::make_index_sequence<size>{});
    }

    template <class Message>
    static constexpr size_t index_of() NOEXCEPT
    {
        return to_index_of<Message>(std::make_index_sequence<size>{});
    }

    static rpc::any_t to_any(size_t index,
        const std::span<const uint8_t>& data) NOEXCEPT
    {
        static constexpr auto table = make_deserializers(
            std::make_index_sequence<size>{});

        return index < size ? table.at(index)(data) : rpc::any_t{};
    }

    static system::chunk_ptr to_frame(size_t index,
        const rpc::any_t& message) NOEXCEPT
    {
        static constexpr auto table = make_serializers(
            std::make_index_sequence<size>{});

        return index < size ? table.at(index)(message) : system::chunk_ptr{};
    }

    static bool to_frame(size_t index, const rpc::any_t& message,
        const system::data_slab& data) NOEXCEPT
    {
        static constexpr auto table = make_framers(
            std::make_index_sequence<size>{});

        return index < size && table.at(index)(message, data);
    }
};

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
