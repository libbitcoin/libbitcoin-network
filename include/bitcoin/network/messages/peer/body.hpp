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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_PEER_BODY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_PEER_BODY_HPP

#include <span>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/header.hpp>
#include <bitcoin/network/messages/http/message.hpp>
#include <bitcoin/network/messages/http/http.hpp>
#include <bitcoin/network/messages/peer/heading.hpp>
#include <bitcoin/network/messages/rpc/model.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

/// http body for bitcoin p2p messages.
struct BCT_API body
{
    /// Content passed to/from the reader/writer via request/response.
    struct value_type
    {
        /// Parse context, stamped by the channel (read in).
        uint32_t magic{};
        uint32_t version{};
        bool witness{};
        bool checksum{};
        size_t maximum{};

        /// Parse fault detail (read out).
        code fault{};

        /// Parsed message heading (read out).
        heading head{};

        /// Type-erased deserialized message (read out).
        rpc::any_t payload{};

        /// Type-erased message with its registry index (write in).
        rpc::any_t message{};
        size_t index{};
        size_t size{};

        /// Serialized v1 frame (write out), unless written to buffer.
        system::chunk_cptr data{};

        /// Writer serialization buffer, owned by the socket.
        mutable http::flat_buffer* buffer{};
    };

    class BCT_API reader
    {
    public:
        using buffer_type = io::const_buffer;

        inline explicit reader(value_type& value) NOEXCEPT
          : value_{ value }
        {
        }

        inline reader(value_type& value, system::data_chunk& payload) NOEXCEPT
          : value_{ value }, payload_{ &payload }
        {
        }

        template <bool IsRequest>
        inline reader(http::header<IsRequest>&,
            value_type& value) NOEXCEPT
          : reader{ value }
        {
        }

        void init(const http::length_type& length, code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, code& ec) NOEXCEPT;
        void finish(code& ec) NOEXCEPT;
        bool done() const NOEXCEPT;

        /// Accept the payload of the identified message (v2).
        void put(uint8_t identifier, const std::string& command,
            const std::span<const uint8_t>& payload, code& ec) NOEXCEPT;

        /// Bytes required to advance the parse (zero when done).
        size_t need() const NOEXCEPT;

    protected:
        value_type& value_;

        // Parse source, null when constructed without a caller buffer (put).
        const system::data_chunk* payload_{};

    private:
        bool accept(const std::span<const uint8_t>& payload,
            code& ec) NOEXCEPT;

        size_t need_{};
        bool headed_{};
        bool done_{};
    };

    class BCT_API writer
    {
    public:
        using const_buffers_type = io::const_buffer;
        using out_buffer = http::get_buffer<const_buffers_type>;

        inline explicit writer(value_type& value) NOEXCEPT
          : value_{ value }
        {
        }

        template <bool IsRequest>
        inline explicit writer(http::header<IsRequest>&,
            value_type& value) NOEXCEPT
          : value_{ value }
        {
        }

        void init(code& ec) NOEXCEPT;
        out_buffer get(code& ec) NOEXCEPT;
        bool done() const NOEXCEPT;

    protected:
        value_type& value_;

    private:
        const_buffers_type bytes_{};
        bool done_{};
    };
};

using frame = body::value_type;
using frame_ptr = std::shared_ptr<frame>;
using frame_cptr = std::shared_ptr<const frame>;

} // namespace peer
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
