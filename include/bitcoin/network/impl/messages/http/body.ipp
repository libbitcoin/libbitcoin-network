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

namespace libbitcoin {
namespace network {
namespace http {

inline bool is_attachment(auto& head) NOEXCEPT
{
    const auto& disposition = head[field::content_disposition];
    const auto content = system::ascii_to_lower(disposition);
    return content.find("filename=") != std::string::npos;
}

// reader
// ----------------------------------------------------------------------------

template <class Body>
inline reader_variant reader_from_body(auto& head,
    variant_payload& pay) NOEXCEPT
{
    BC_ASSERT(pay.inner.has_value());
    using reader = typename Body::reader;
    using value = typename Body::value_type;
    return reader_variant{ std::in_place_type<reader>, head,
        std::get<value>(pay.inner.value()) };
}

/// Select reader based on content-type header.
inline reader_variant to_reader(auto& head, variant_payload& pay) NOEXCEPT
{
    switch (content_mime_type(head[field::content_type]))
    {
        case mime_type::application_json:
        {
            pay.inner = json_value{};
            return reader_from_body<json_body>(head, pay);
        }
        case mime_type::text_plain:
        {
            pay.inner = string_value{};
            return reader_from_body<string_body>(head, pay);
        }
        case mime_type::application_octet:
        {
            if (is_attachment(head))
            {
                pay.inner = file_value{};
                return reader_from_body<file_body>(head, pay);
            }
            else
            {
                pay.inner = data_value{};
                return reader_from_body<data_body>(head, pay);
            }
        }
        default:
        {
            pay.inner = empty_value{};
            return reader_from_body<empty_body>(head, pay);
        }
    }
}

template <bool IsRequest, class Fields>
inline body::reader::reader(header<IsRequest, Fields>& header,
    value_type& payload) NOEXCEPT
  : reader_{ to_reader(header, payload) }
{
}

inline void body::reader::init(const length_type& length,
    error_code& ec) NOEXCEPT
{
    std::visit(overload
    {
        [&](auto& read) NOEXCEPT
        {
            try
            {
                read.init(length, ec);
            }
            catch (...)
            {
                ec = error::to_boost_code(error::boost_error_t::io_error);
            }
        }
    }, reader_);
}

inline size_t body::reader::put(const buffer_type& buffer,
    error_code& ec) NOEXCEPT
{
    return std::visit(overload
    {
        [&](auto& read) NOEXCEPT
        {
            try
            {
                return read.put(buffer, ec);
            }
            catch (...)
            {
                ec = error::to_boost_code(error::boost_error_t::io_error);
                return size_t{};
            }
        }
    }, reader_);
}

inline void body::reader::finish(error_code& ec) NOEXCEPT
{
    return std::visit(overload
    {
        [&](auto& read) NOEXCEPT
        {
            try
            {
                return read.finish(ec);
            }
            catch (...)
            {
                ec = error::to_boost_code(error::boost_error_t::io_error);
            }
        }
    }, reader_);
}

// writer
// ----------------------------------------------------------------------------

template <class Body>
inline writer_variant writer_from_body(auto& head,
    const variant_payload& pay) NOEXCEPT
{
    BC_ASSERT(pay.inner.has_value());
    using writer = typename Body::writer;
    using value = typename Body::value_type;
    return writer_variant{ std::in_place_type<writer>, head,
        std::get<value>(pay.inner.value()) };
}

/// Create writer matching the caller-defined body.inner (variant) type.
inline writer_variant to_writer(auto& head,
    const variant_payload& pay) NOEXCEPT
{
    // Caller should have set inner, otherwise set it to empty (it's mutable).
    if (!pay.inner.has_value())
        pay.inner = empty_value{};

    return std::visit(overload
    {
        [&](const json_value&) NOEXCEPT
        {
            return writer_from_body<json_body>(head, pay);
        },
        [&](const string_value&) NOEXCEPT
        {
            return writer_from_body<string_body>(head, pay);
        },
        [&](const data_value&) NOEXCEPT
        {
            return writer_from_body<data_body>(head, pay);
        },
        [&](const file_value&) NOEXCEPT
        {
            return writer_from_body<file_body>(head, pay);
        },
        [&](const empty_value&) NOEXCEPT
        {
            return writer_from_body<empty_body>(head, pay);
        }
    }, pay.inner.value());
}

template <bool IsRequest, class Fields>
inline body::writer::writer(header<IsRequest, Fields>& header,
    const value_type& payload) NOEXCEPT
  : writer_{ to_writer(header, payload) }
{
}

inline void body::writer::init(error_code& ec) NOEXCEPT
{
    return std::visit(overload
    {
        [&] (auto& write) NOEXCEPT
        {
            try
            {
                write.init(ec);
            }
            catch (...)
            {
                ec = error::to_boost_code(error::boost_error_t::io_error);
            }
        }
    }, writer_);
}

inline body::writer::out_buffer body::writer::get(error_code& ec) NOEXCEPT
{
    return std::visit(overload
    {
        [&](auto& write) NOEXCEPT
        {
            try
            {
                return write.get(ec);
            }
            catch (...)
            {
                ec = error::to_boost_code(error::boost_error_t::io_error);
                return out_buffer{};
            }
        }
    }, writer_);
}

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
