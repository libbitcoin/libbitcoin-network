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
    /// Required by boost::beast.
    using value_type = payload;

    /// Required: reader for incoming data.
    class reader
    {
    public:
        /// MUST be boost::optional.
        using length_t = boost::optional<uint64_t>;

        /// Called once per message, header is mutable.
        template <bool IsRequest, class Fields = http::fields>
        explicit reader(http::header<IsRequest, Fields>&,
            value_type& payload) NOEXCEPT
          : payload_{ payload }
        {
        }

        /// Called once at start of body deserialization.
        void init(length_t content_length, error_code& ec) NOEXCEPT;

        /// Called zero or more times with incoming (from wire) buffers.
        /// Bytes are consumed from buffers and parsed into json object.
        /// Fails if not all characters are consumed by the logical parse.
        size_t put(asio::const_buffer buffer, error_code& ec) NOEXCEPT;

        /// Called once at the end of body deserialization, signals completion.
        void finish(error_code& ec) NOEXCEPT;

    private:
        value_type& payload_;
        length_t expected_{};
        size_t total_{};
        Parser parser_{};
    };

    /// Required: writer for outgoing data.
    class writer
    {
    public:
        /// Required by boost::beast.
        using const_buffers_type = asio::const_buffer;

        /// MUST be boost::optional.
        using buffers_t = boost::optional<std::pair<const_buffers_type, bool>>;

        /// Called once per message, header is mutable.
        template <bool IsRequest, class Fields = http::fields>
        explicit writer(http::header<IsRequest, Fields>&,
            const value_type& payload) NOEXCEPT
          : payload_{ payload },
            buffer_{ payload.size },
            serializer_{ payload.model.storage() }
        {
        }

        /// Called once at start of message serialization.
        void init(error_code& ec) NOEXCEPT;

        /// Called one or more times to get outgoing (to wire) buffers.
        buffers_t get(error_code& ec) NOEXCEPT;

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
