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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_VARIANT_BODY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_VARIANT_BODY_HPP

#include <optional>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/body.hpp>
#include <bitcoin/network/messages/http/http.hpp>

// TODO: relies on http::header, length_type, get_buffer, mime_type.

namespace libbitcoin {
namespace network {
namespace variant {

using empty_reader = http::empty_body::reader;
using json_reader = json::body::reader;
using data_reader = http::data_body::reader;
using file_reader = http::file_body::reader;
using span_reader = http::span_body::reader;
using buffer_reader = http::buffer_body::reader;
using string_reader = http::string_body::reader;
using body_reader = std::variant
<
    empty_reader,
    json_reader,
    data_reader,
    file_reader,
    span_reader,
    buffer_reader,
    string_reader
>;

using empty_writer = http::empty_body::writer;
using json_writer = json::body::writer;
using data_writer = http::data_body::writer;
using file_writer = http::file_body::writer;
using span_writer = http::span_body::writer;
using buffer_writer = http::buffer_body::writer;
using string_writer = http::string_body::writer;
using body_writer = std::variant
<
    empty_writer,
    json_writer,
    data_writer,
    file_writer,
    span_writer,
    buffer_writer,
    string_writer
>;

using empty_value = http::empty_body::value_type;
using json_value = json::body::value_type;
using data_value = http::data_body::value_type;
using file_value = http::file_body::value_type;
using span_value = http::span_body::value_type;
using buffer_value = http::buffer_body::value_type;
using string_value = http::string_body::value_type;
using body_value = std::variant
<
    empty_value,
    json_value,
    data_value,
    file_value,
    span_value,
    buffer_value,
    string_value
>;

/// body template for all known message types.
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
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, span_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, buffer_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, string_value, inner_)

        inline bool has_value() const NOEXCEPT
        {
            return inner_.has_value();
        }

        inline body_value& value() NOEXCEPT
        {
            return inner_.value();
        }

        inline const body_value& value() const NOEXCEPT
        {
            return inner_.value();
        }

        template <typename Inner>
        inline bool contains() const NOEXCEPT
        {
            return has_value() && std::holds_alternative<Inner>(value());
        }

        template <typename Inner>
        inline const Inner& get() const NOEXCEPT
        {
            BC_ASSERT(contains<Inner>());
            return std::get<Inner>(value());
        }

    private:
        std::optional<body_value> inner_{};
    };

    class reader
    {
    public:
        using buffer_type = asio::const_buffer;

        template <bool IsRequest, class Fields>
        inline explicit reader(http::message_header<IsRequest, Fields>& header,
            value_type& value) NOEXCEPT
          : reader_{ to_reader(header, value) }
        {
        }

        void init(const http::length_type& length, boost_code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, boost_code& ec) NOEXCEPT;
        void finish(boost_code& ec) NOEXCEPT;

    protected:
        /// Select reader based on content-type header.
        template <class Header>
        inline static body_reader to_reader(Header& header,
            value_type& value) NOEXCEPT
        {
            switch (http::content_mime_type(header))
            {
                case http::mime_type::application_json:
                    value = json_value{};
                    break;
                case http::mime_type::text_plain:
                    value = string_value{};
                    break;
                case http::mime_type::application_octet_stream:
                    if (http::has_attachment(header))
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
                    return body_reader{ empty_reader{ header, value } };
                },
                [&](json_value& value) NOEXCEPT
                {
                    // json_reader not copy or assignable (by contained parser).
                    // So *requires* in-place construction for variant populate.
                    return body_reader{ std::in_place_type<json_reader>,
                        header, value };
                },
                [&](data_value& value) NOEXCEPT
                {
                    return body_reader{ data_reader{ header, value } };
                },
                [&](file_value& value) NOEXCEPT
                {
                    return body_reader{ file_reader{ header, value } };
                },
                [&](span_value& value) NOEXCEPT
                {
                    return body_reader{ span_reader{ header, value } };
                },
                [&](buffer_value& value) NOEXCEPT
                {
                    return body_reader{ buffer_reader{ header, value } };
                },
                [&](string_value& value) NOEXCEPT
                {
                    return body_reader{ string_reader{ header, value } };
                }
            }, value.value());
        }

    private:
        body_reader reader_;
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = http::get_buffer<const_buffers_type>;

        template <bool IsRequest, class Fields>
        inline explicit writer(http::message_header<IsRequest, Fields>& header,
            value_type& value) NOEXCEPT
          : writer_{ to_writer(header, value) }
        {
        }

        void init(boost_code& ec) NOEXCEPT;
        out_buffer get(boost_code& ec) NOEXCEPT;

    protected:
        /// Create writer matching the caller-defined body inner variant type.
        template <class Header>
        inline static body_writer to_writer(Header& header,
            value_type& value) NOEXCEPT
        {
            // Caller should have set optional<>, otherwise set to empty_value.
            if (!value.has_value())
                value = empty_value{};

            return std::visit(overload
            {
                [&](empty_value& value) NOEXCEPT
                {
                    return body_writer{ empty_writer{ header, value } };
                },
                [&](json_value& value) NOEXCEPT
                {
                    // json_writer is not movable (by contained serializer).
                    // So *requires* in-place construction for variant populate.
                    return body_writer{ std::in_place_type<json_writer>,
                        header, value };
                },
                [&](data_value& value) NOEXCEPT
                {
                    return body_writer{ data_writer{ header, value } };
                },
                [&](file_value& value) NOEXCEPT
                {
                    return body_writer{ file_writer{ header, value } };
                },
                [&](span_value& value) NOEXCEPT
                {
                    return body_writer{ span_writer{ header, value } };
                },
                [&](buffer_value& value) NOEXCEPT
                {
                    return body_writer{ buffer_writer{ header, value } };
                },
                [&](string_value& value) NOEXCEPT
                {
                    return body_writer{ string_writer{ header, value } };
                }
            }, value.value());
        }

    private:
        body_writer writer_;
    };
};

} // namespace variant
} // namespace network
} // namespace libbitcoin

#endif
