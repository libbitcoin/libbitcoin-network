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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_HEAD_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_HEAD_HPP

#include <utility>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

template <bool IsRequest>
struct head
{
    // forward declaration
    class value_type;

    class reader
    {
    public:
        using buffer_type = asio::const_buffer;

        inline explicit reader(value_type& value) NOEXCEPT
          : value_{ value }
        {
        }

        inline void init(const length_type& length, boost_code& ec) NOEXCEPT;
        inline size_t put(const buffer_type& buffer, boost_code& ec) NOEXCEPT;
        inline void finish(boost_code& ec) NOEXCEPT;

    private:
        value_type& value_;
        empty_parser<IsRequest> parser_{};
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        inline explicit writer(value_type& value) NOEXCEPT
          : value_{ value }, serializer_{ value.message_ }
        {
        }

        inline void init(boost_code& ec) NOEXCEPT;
        inline out_buffer get(boost_code& ec) NOEXCEPT;

    private:
        static constexpr size_t default_buffer = 1024;
        value_type& value_;
        empty_serializer<IsRequest> serializer_;
    };

    struct value_type
    {
        using header_type = header<IsRequest>;
        using message_type = empty_message<IsRequest>;

        value_type() NOEXCEPT
          : value_type(message_type{})
        {
        }

        value_type(header_type&& header) NOEXCEPT
          : message_{ std::move(header) }, header{ message_ }
        {
        }

        /// Writer serialization buffer (max size, allocated on write).
        mutable flat_buffer_ptr buffer{};

    private:
        friend typename head<IsRequest>::writer;
        message_type message_;
    public:
        header_type& header;
    };
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <bool IsRequest>
#define CLASS head<IsRequest>

#include <bitcoin/network/impl/messages/http/head.ipp>

#undef CLASS
#undef TEMPLATE

#endif
