/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_HPP

#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace json {

/// boost::beast::http body for JSON messages.
/// Because of the parser and serializer members, neither the reader nor writer
/// is movable and as such must be in-place contructed (e.g. variant contruct).
struct BCT_API body
{
    
    /// Content passed to/from reader/writer via request/response.
    /// `static uint64_t size(const value_type&)` must be defined for beast to 
    /// produce `content_length`, otherwise the response is chunked. Predeter-
    /// mining size would have the effect of eliminating the benefit of
    /// streaming serialize.
    struct value_type
    {
        /// JSON DOM.
        boost::json::value model{};

        /// Used by channel to resize reusable buffer.
        size_t size_hint{};

        /// Writer serialization buffer (max size, allocated on write).
        mutable http::flat_buffer_ptr buffer{};
    };

    class reader
    {
    public:
        using buffer_type = asio::const_buffer;

        template <bool IsRequest, class Fields>
        inline explicit reader(http::header<IsRequest, Fields>&,
            value_type& value) NOEXCEPT
          : value_{ value }
        {
        }

        void init(const http::length_type& length, boost_code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, boost_code& ec) NOEXCEPT;
        void finish(boost_code& ec) NOEXCEPT;

    private:
        value_type& value_;
        size_t total_{};
        http::length_type expected_{};
        boost::json::stream_parser parser_{};
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = http::get_buffer<const_buffers_type>;

        template <bool IsRequest, class Fields>
        inline explicit writer(http::header<IsRequest, Fields>&,
            value_type& value) NOEXCEPT
          : value_{ value },
            serializer_{ value.model.storage() }
        {
        }

        void init(boost_code& ec) NOEXCEPT;
        out_buffer get(boost_code& ec) NOEXCEPT;

    private:
        static constexpr size_t default_buffer = 4096;
        const value_type& value_;
        boost::json::serializer serializer_;
    };
};

} // namespace json
} // namespace network
} // namespace libbitcoin

namespace libbitcoin {
namespace network {
namespace http {
    
using json_body = json::body;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
