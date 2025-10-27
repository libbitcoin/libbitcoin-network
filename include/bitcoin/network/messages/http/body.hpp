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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_BODY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_BODY_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// Content passed to/from reader/writer via request/response.
struct payload
{
};

/// boost::beast::http body template for all message types.
struct body
{
    using value_type = payload;

    class reader
    {
    public:
        using buffer_type = asio::const_buffer;

        template <bool IsRequest, class Fields>
        inline explicit reader(header<IsRequest, Fields>& header,
            value_type& payload) NOEXCEPT;

        inline void init(const length_type& length, error_code& ec) NOEXCEPT;
        inline size_t put(const buffer_type& buffer, error_code& ec) NOEXCEPT;
        inline void finish(error_code& ec) NOEXCEPT;

    ////private:
    ////    value_type& payload_;
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest, class Fields>
        inline explicit writer(header<IsRequest, Fields>& header,
            const value_type& payload) NOEXCEPT;

        inline void init(error_code& ec) NOEXCEPT;
        inline out_buffer get(error_code& ec) NOEXCEPT;

    ////private:
    ////    const value_type& payload_;
    };
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/messages/http/body.ipp>

#endif
