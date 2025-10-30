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
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/mime_type.hpp>
#include <bitcoin/network/messages/http/fields.hpp>
#include <bitcoin/network/messages/json/body.hpp>

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

using empty_value = empty_body::value_type;
using json_value = json_body::value_type;
using data_value = data_body::value_type;
using file_value = file_body::value_type;
using string_value = string_body::value_type;
using variant_value = std::variant
<
    empty_value,
    json_value,
    data_value,
    file_value,
    string_value
>;

/// boost::beast::http body template for all known message types.
/// This encapsulates a variant of supported body types, selects a type upon
/// reader or writer construction, and then passes all calls through to it.
struct BCT_API body
{
    /// No size(), forces chunked encoding for all types.
    /// The pass-thru body(), reader populates in construct.
    struct value_type
    {
        /// Allow default construct (empty optional).
        value_type() NOEXCEPT = default;

        /// Forwarding constructors for in-place variant construction.
        FORWARD_VARIANT_CONSTRUCT(value_type, inner_)
        FORWARD_VARIANT_ASSIGNMENT(value_type, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, empty_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, json_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, data_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, file_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, string_value, inner_)

        bool has_value() NOEXCEPT
        {
            return inner_.has_value();
        }

        variant_value& value() NOEXCEPT
        {
            return inner_.value();
        }

        const variant_value& value() const NOEXCEPT
        {
            return inner_.value();
        }

    private:
        std::optional<variant_value> inner_{};
    };

    class reader
    {
    public:
        using buffer_type = asio::const_buffer;

        template <bool IsRequest, class Fields>
        inline explicit reader(header<IsRequest, Fields>& header,
            value_type& value) NOEXCEPT
          : reader_{ to_reader(header, value) }
        {
        }

        void init(const length_type& length, error_code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, error_code& ec) NOEXCEPT;
        void finish(error_code& ec) NOEXCEPT;

    protected:
        /// Select reader based on content-type header.
        template <class Header>
        inline static variant_reader to_reader(Header& header,
            value_type& value) NOEXCEPT
        {
            switch (content_mime_type(header))
            {
                case mime_type::application_json:
                    value = json_value{};
                    break;
                case mime_type::text_plain:
                    value = string_value{};
                    break;
                case mime_type::application_octet_stream:
                    if (has_attachment(header))
                        value = file_value{};
                    else
                        value = data_value{};
                    break;
                default:
                    value = empty_value{};
            }

            return std::visit(overload
            {
                [&](empty_value& value) NOEXCEPT
                {
                    return variant_reader{ empty_reader{ header, value } };
                },
                [&](json_value& value) NOEXCEPT
                {
                    // json_reader not copy or assignable (by contained parser).
                    // So *requires* in-place construction for variant populate.
                    return variant_reader{ std::in_place_type<json_reader>,
                        header, value };
                },
                [&](data_value& value) NOEXCEPT
                {
                    return variant_reader{ data_reader{ header, value } };
                },
                [&](file_value& value) NOEXCEPT
                {
                    return variant_reader{ file_reader{ header, value } };
                },
                [&](string_value& value) NOEXCEPT
                {
                    return variant_reader{ string_reader{ header, value } };
                }
            }, value.value());
        }

    private:
        variant_reader reader_;
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest, class Fields>
        inline explicit writer(header<IsRequest, Fields>& header,
            value_type& value) NOEXCEPT
          : writer_{ to_writer(header, value) }
        {
        }

        void init(error_code& ec) NOEXCEPT;
        out_buffer get(error_code& ec) NOEXCEPT;

    protected:
        /// Create writer matching the caller-defined body inner variant type.
        template <class Header>
        inline static variant_writer to_writer(Header& header,
            value_type& value) NOEXCEPT
        {
            // Caller should have set optional<>, otherwise set to empty_value.
            if (!value.has_value())
                value = empty_value{};

            return std::visit(overload
            {
                [&](empty_value& value) NOEXCEPT
                {
                    return variant_writer{ empty_writer{ header, value } };
                },
                [&](json_value& value) NOEXCEPT
                {
                    // json_writer is not movable (by contained serializer).
                    // So *requires* in-place construction for variant populate.
                    return variant_writer{ std::in_place_type<json_writer>,
                        header, value };
                },
                [&](data_value& value) NOEXCEPT
                {
                    return variant_writer{ data_writer{ header, value } };
                },
                [&](file_value& value) NOEXCEPT
                {
                    return variant_writer{ file_writer{ header, value } };
                },
                [&](string_value& value) NOEXCEPT
                {
                    return variant_writer{ string_writer{ header, value } };
                }
            }, value.value());
        }

    private:
        variant_writer writer_;
    };
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
