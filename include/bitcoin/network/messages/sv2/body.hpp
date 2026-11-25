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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_SV2_BODY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_SV2_BODY_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/http.hpp>
#include <bitcoin/network/messages/rpc/model.hpp>
#include <bitcoin/network/messages/sv2/heading.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

/// http body for stratum v2 messages.
struct BCT_API body
{
    /// Content passed to/from the reader/writer via request/response.
    struct value_type
    {
        /// Parse context, stamped by the channel (read in).
        size_t maximum{};

        /// Exact requires that each buffer read is one whole frame.
        bool exact{};

        /// Parse fault detail (read out).
        code fault{};

        /// Parsed message heading (read out).
        heading head{};

        /// Type-erased deserialized message (read out), empty if the
        /// message is not registered.
        rpc::any_t payload{};

        /// Type-erased message with its registry index (write in).
        rpc::any_t message{};
        size_t index{};
        size_t size{};

        /// Serialized frame (write out), unless written to buffer.
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

    protected:
        value_type& value_;

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

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
