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

#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/mime_type.hpp>
#include <bitcoin/network/messages/http/fields.hpp>

namespace libbitcoin {
namespace network {
namespace http {

// These are the templated construction methods, for passthrough see body.cpp.

// reader
// ----------------------------------------------------------------------------
    
template <class Body, class Fields>
reader_variant body::reader::reader_from_body(Fields& header,
    variant_payload& payload) NOEXCEPT
{
    BC_ASSERT(payload.inner.has_value());
    using reader = typename Body::reader;
    using value = typename Body::value_type;
    return reader_variant{ std::in_place_type<reader>, header,
        std::get<value>(payload.inner.value()) };
}

/// Select reader based on content-type header.
template <class Fields>
reader_variant body::reader::to_reader(Fields& header,
    variant_payload& payload) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    switch (content_mime_type(header[field::content_type]))
    BC_POP_WARNING()
    {
        case mime_type::application_json:
        {
            payload.inner = json_value{};
            return reader_from_body<json_body>(header, payload);
        }
        case mime_type::text_plain:
        {
            payload.inner = string_value{};
            return reader_from_body<string_body>(header, payload);
        }
        case mime_type::application_octet:
        {
            if (has_attachment(header))
            {
                payload.inner = file_value{};
                return reader_from_body<file_body>(header, payload);
            }
            else
            {
                payload.inner = data_value{};
                return reader_from_body<data_body>(header, payload);
            }
        }
        default:
        {
            payload.inner = empty_value{};
            return reader_from_body<empty_body>(header, payload);
        }
    }
}

template <bool IsRequest, class Fields>
body::reader::reader(header<IsRequest, Fields>& header,
    value_type& payload) NOEXCEPT
  : reader_{ to_reader(header, payload) }
{
}

// writer
// ----------------------------------------------------------------------------

template <class Body, class Fields>
writer_variant body::writer::writer_from_body(Fields& header,
    const variant_payload& payload) NOEXCEPT
{
    BC_ASSERT(payload.inner.has_value());
    using writer = typename Body::writer;
    using value = typename Body::value_type;
    return writer_variant{ std::in_place_type<writer>, header,
        std::get<value>(payload.inner.value()) };
}

/// Create writer matching the caller-defined body.inner (variant) type.
template <class Fields>
writer_variant body::writer::to_writer(Fields& header,
    const variant_payload& payload) NOEXCEPT
{
    // Caller should have set inner, otherwise set it to empty (it's mutable).
    if (!payload.inner.has_value())
        payload.inner = empty_value{};

    return std::visit(overload
    {
        [&](const json_value&) NOEXCEPT
        {
            return writer_from_body<json_body>(header, payload);
        },
        [&](const string_value&) NOEXCEPT
        {
            return writer_from_body<string_body>(header, payload);
        },
        [&](const data_value&) NOEXCEPT
        {
            return writer_from_body<data_body>(header, payload);
        },
        [&](const file_value&) NOEXCEPT
        {
            return writer_from_body<file_body>(header, payload);
        },
        [&](const empty_value&) NOEXCEPT
        {
            return writer_from_body<empty_body>(header, payload);
        }
    }, payload.inner.value());
}

template <bool IsRequest, class Fields>
body::writer::writer(header<IsRequest, Fields>& header,
    const value_type& payload) NOEXCEPT
  : writer_{ to_writer(header, payload) }
{
}

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
