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

#include <optional>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/body.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using json_body = json::body<json::parser, json::serializer>;

using empty_value = empty_body::value_type;
using json_value = json_body::value_type;
using data_value = data_body::value_type;
using file_value = file_body::value_type;
using string_value = string_body::value_type;

using empty_reader = empty_body::reader;
using json_reader = json_body::reader;
using data_reader = data_body::reader;
using file_reader = file_body::reader;
using string_reader = string_body::reader;

using empty_writer = empty_body::writer;
using json_writer = json_body::writer;
using data_writer = data_body::writer;
using file_writer = file_body::writer;
using string_writer = string_body::writer;

using value_variant = std::variant
<
    empty_value,
    json_value,
    data_value,
    file_value,
    string_value
>;

using reader_variant = std::variant
<
    empty_reader,
    json_reader,
    data_reader,
    file_reader,
    string_reader
>;

using writer_variant = std::variant
<
    empty_writer,
    json_writer,
    data_writer,
    file_writer,
    string_writer
>;

/// No size(), forces chunked encoding for all types.
/// The pass-thru body(), reader populates in construct.
struct variant_payload
{
    std::optional<value_variant> inner{};
};

/// boost::beast::http body template for all message types.
/// This encapsulates a variant of supported body types, selects a type upon
/// reader or writer construction, and then passes all calls through to it.
struct body
{
    using value_type = variant_payload;

    class reader
    {
    public:
        using buffer_type = asio::const_buffer;

        template <bool IsRequest, class Fields>
        explicit reader(header<IsRequest, Fields>& header,
            value_type& payload) NOEXCEPT;

        void init(const length_type& length, error_code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, error_code& ec) NOEXCEPT;
        void finish(error_code& ec) NOEXCEPT;

    protected:
        template <class Header>
        static reader_variant to_reader(Header& header,
            variant_payload& payload) NOEXCEPT;

    private:
        reader_variant reader_;
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest, class Fields>
        explicit writer(header<IsRequest, Fields>& header,
            value_type& payload) NOEXCEPT;

        void init(error_code& ec) NOEXCEPT;
        out_buffer get(error_code& ec) NOEXCEPT;

    protected:
        template <class Header>
        static writer_variant to_writer(Header& header,
            variant_payload& payload) NOEXCEPT;

    private:
        writer_variant writer_;
    };
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/messages/http/body.ipp>

#endif
