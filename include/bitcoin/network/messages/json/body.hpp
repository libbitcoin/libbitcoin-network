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
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

/// Content passed to/from reader/writer via request/response.
struct payload
{
    /// JSON DOM.
    json_value model{};

    /// writer: buffer size, reader: bytes read.
    size_t size{ 1024 };

    /////// Not so efficient.
    ////static uint64_t size(const payload& self) NOEXCEPT
    ////{
    ////    return boost::json::serialize(self.model).size();
    ////}
};

/// boost::beast::http body template for JSON messages.
template <class Parser, class Serializer>
struct body
{
    using value_type = payload;

    class reader
    {
    public:
        using buffer_type = asio::const_buffer;

        template <bool IsRequest, class Fields>
        explicit reader(http::header<IsRequest, Fields>&,
            value_type& payload) NOEXCEPT
          : payload_{ payload }
        {
        }

        void init(const http::length_type& length, error_code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, error_code& ec) NOEXCEPT;
        void finish(error_code& ec) NOEXCEPT;

    private:
        value_type& payload_;
        size_t total_{};
        Parser parser_{};
        http::length_type expected_{};
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = http::get_buffer<const_buffers_type>;

        template <bool IsRequest, class Fields>
        explicit writer(http::header<IsRequest, Fields>&,
            const value_type& payload) NOEXCEPT
          : payload_{ payload },
            buffer_{ payload.size },
            serializer_{ payload.model.storage() }
        {
        }

        void init(error_code& ec) NOEXCEPT;
        out_buffer get(error_code& ec) NOEXCEPT;

    private:
        const value_type& payload_;
        http::flat_buffer buffer_;
        Serializer serializer_;
    };
};

using parser = boost::json::stream_parser;
using serializer = boost::json::serializer;

} // namespace json
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <class Parser, class Serializer>
#define CLASS body<Parser, Serializer>

#include <bitcoin/network/impl/messages/json/body.ipp>

#undef CLASS
#undef TEMPLATE

#endif
