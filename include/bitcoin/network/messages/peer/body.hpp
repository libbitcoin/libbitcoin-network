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

#include <memory>
#include <span>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/peer/heading.hpp>
#include <bitcoin/network/messages/rpc/model.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace peer {

/// boost::beast::http body for bitcoin p2p messages.
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

        /// Serialized frame from peer::serialize (write in).
        system::chunk_cptr data{};
    };

    class BCT_API reader
    {
    public:
        using buffer_type = asio::const_buffer;

        inline explicit reader(value_type& value) NOEXCEPT
          : value_{ value }
        {
        }

        inline reader(value_type& value, system::data_chunk& payload) NOEXCEPT
          : value_{ value }, payload_{ &payload }
        {
        }

        template <bool IsRequest, class Fields>
        inline reader(http::message_header<IsRequest, Fields>&,
            value_type& value) NOEXCEPT
          : reader{ value }
        {
        }

        void init(const http::length_type& length, boost_code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, boost_code& ec) NOEXCEPT;
        void finish(boost_code& ec) NOEXCEPT;
        bool done() const NOEXCEPT;

        /// Accept the payload of the identified message (v2).
        void put(uint8_t identifier, const std::string& command,
            const std::span<const uint8_t>& payload, boost_code& ec) NOEXCEPT;

        /// Bytes required to advance the parse (zero when done).
        size_t need() const NOEXCEPT;

    protected:
        value_type& value_;

        // Parse source, null when constructed without a caller buffer (put).
        const system::data_chunk* payload_{};

    private:
        bool accept(const std::span<const uint8_t>& payload,
            boost_code& ec) NOEXCEPT;

        size_t need_{};
        bool headed_{};
        bool done_{};
    };

    class BCT_API writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = http::get_buffer<const_buffers_type>;

        inline explicit writer(value_type& value) NOEXCEPT
          : value_{ value }
        {
        }

        template <bool IsRequest, class Fields>
        inline explicit writer(http::message_header<IsRequest, Fields>&,
            value_type& value) NOEXCEPT
          : value_{ value }
        {
        }

        void init(boost_code& ec) NOEXCEPT;
        out_buffer get(boost_code& ec) NOEXCEPT;
        bool done() const NOEXCEPT;

    protected:
        value_type& value_;

    private:
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
