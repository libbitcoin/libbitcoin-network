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

#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/body.hpp>
#include <bitcoin/network/messages/http/payload.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using empty_reader = empty_body::reader;
using json_reader = json_body::reader;
using data_reader = data_body::reader;
using file_reader = file_body::reader;
using string_reader = string_body::reader;
using variant_reader = std::variant
<
    empty_reader,
    json_reader,
    data_reader,
    file_reader,
    string_reader
>;

using empty_writer = empty_body::writer;
using json_writer = json_body::writer;
using data_writer = data_body::writer;
using file_writer = file_body::writer;
using string_writer = string_body::writer;
using variant_writer = std::variant
<
    empty_writer,
    json_writer,
    data_writer,
    file_writer,
    string_writer
>;

/// boost::beast::http body template for all known message types.
/// This encapsulates a variant of supported body types, selects a type upon
/// reader or writer construction, and then passes all calls through to it.
struct BCT_API body
{
    using value_type = payload;

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
        static variant_reader to_reader(Header& header,
            http::payload& value) NOEXCEPT;

    private:
        variant_reader reader_;
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest, class Fields>
        explicit writer(header<IsRequest, Fields>& header,
            value_type& value) NOEXCEPT;

        void init(error_code& ec) NOEXCEPT;
        out_buffer get(error_code& ec) NOEXCEPT;

    protected:
        template <class Header>
        static variant_writer to_writer(Header& header,
            http::payload& value) NOEXCEPT;

    private:
        variant_writer writer_;
    };
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/messages/http/body.ipp>

#endif
