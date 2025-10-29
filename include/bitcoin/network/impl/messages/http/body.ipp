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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_BODY_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_BODY_IPP

#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/mime_type.hpp>
#include <bitcoin/network/messages/http/fields.hpp>
#include <bitcoin/network/messages/http/body.hpp>

namespace libbitcoin {
namespace network {
namespace http {

// These are the templated construction methods, for passthrough see body.cpp.

// reader
// ----------------------------------------------------------------------------

// static
// Select reader based on content-type header.
template <class Header>
variant_reader body::reader::to_reader(Header& header,
    payload& value) NOEXCEPT
{
    switch (content_mime_type(header))
    {
        case mime_type::application_json:
            value.inner = json_value{};
            break;
        case mime_type::text_plain:
            value.inner = string_value{};
            break;
        case mime_type::application_octet_stream:
            if (has_attachment(header))
                value.inner = file_value{};
            else
                value.inner = data_value{};
            break;
        default:
            value.inner = empty_value{};
    }

    return std::visit(overload
    {
        [&](empty_value& value) NOEXCEPT
        {
            return variant_reader{ empty_reader{ header, value } };
        },
        [&](json_value& value) NOEXCEPT
        {
            // json_reader is not copy or assignable (by contained parser).
            // So *requires* in-place construction for variant population.
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
    }, value.inner.value());
}

template <bool IsRequest, class Fields>
body::reader::reader(header<IsRequest, Fields>& header,
    value_type& value) NOEXCEPT
  : reader_{ to_reader(header, value) }
{
}

// writer
// ----------------------------------------------------------------------------

// static
// Create writer matching the caller-defined body.inner (variant) type.
template <class Header>
variant_writer body::writer::to_writer(Header& header,
    payload& value) NOEXCEPT
{
    // Caller should have set inner, otherwise set it to empty.
    if (!value.inner.has_value())
        value.inner = empty_value{};

    return std::visit(overload
    {
        [&](empty_value& value) NOEXCEPT
        {
            return variant_writer{ empty_writer{ header, value } };
        },
        [&](json_value& value) NOEXCEPT
        {
            // json_writer is not movable (by contained serializer).
            // So *requires* in-place construction for variant population.
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
    }, value.inner.value());
}

template <bool IsRequest, class Fields>
body::writer::writer(header<IsRequest, Fields>& header,
    value_type& value) NOEXCEPT
  : writer_{ to_writer(header, value) }
{
}

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
