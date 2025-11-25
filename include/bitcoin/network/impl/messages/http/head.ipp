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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_HEAD_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_HEAD_IPP

#include <memory>
#include <utility>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

// http::head::reader
// ----------------------------------------------------------------------------

TEMPLATE
inline void CLASS::reader::init(const length_type& length,
    boost_code& ec) NOEXCEPT
{
    using namespace system;
    using namespace network::error;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto value = length.get_value_or(zero);
    BC_POP_WARNING()

    if (is_limited<uint32_t>(value))
    {
        ec = to_http_code(http_error_t::buffer_overflow);
        return;
    }

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    parser_.eager(true);
    parser_.header_limit(narrow_cast<uint32_t>(value));
    BC_POP_WARNING()
    ec.clear();
}

TEMPLATE
inline size_t CLASS::reader::put(const buffer_type& buffer,
    boost_code& ec) NOEXCEPT
{
    using namespace network::error;
    size_t parsed{};

    try
    {
        parsed = parser_.put(buffer, ec);
    }
    catch (const boost::system::system_error& e)
    {
        // Primary exception type for parsing operations.
        ec = e.code();
    }
    catch (...)
    {
        // As a catch-all we blame alloc.
        ec = to_http_code(http_error_t::bad_alloc);
    }

    if (ec == http_error_t::need_more)
    {
        ec.clear();
        return buffer.size();
    }

    return ec ? zero : parsed;
}

TEMPLATE
inline void CLASS::reader::finish(boost_code& ec) NOEXCEPT
{
    using namespace network::error;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    ec = parser_.is_header_done() ? boost_code{} : 
        to_http_code(http_error_t::partial_message);
    BC_POP_WARNING()

    if (ec) return;

    try
    {
        value_.header = std::move(parser_.release());
    }
    catch (const boost::system::system_error& e)
    {
        // Primary exception type for parsing operations.
        ec = e.code();
    }
    catch (...)
    {
        // As a catch-all we blame alloc.
        ec = to_http_code(http_error_t::bad_alloc);
    }
}

// http::head::writer
// ----------------------------------------------------------------------------

TEMPLATE
inline void CLASS::writer::init(boost_code& ec) NOEXCEPT
{
    if (!value_.buffer)
    {
        // Caller controls max_size and other buffer behavior by assigning it.
        value_.buffer = system::emplace_shared<flat_buffer>(default_buffer);
    }
    else
    {
        // Caller has assigned the buffer (or just reused the response).
        value_.buffer->consume(value_.buffer->size());
    }

    ec.clear();
}

TEMPLATE
inline CLASS::writer::out_buffer CLASS::writer::get(boost_code& ec) NOEXCEPT
{
    ec.clear();
    if (serializer_.is_done())
        return {};

    using namespace network::error;
    const auto size = value_.buffer->max_size();
    if (is_zero(size))
    {
        ec = to_http_code(http_error_t::buffer_overflow);
        return {};
    }

    try
    {
        // Always prepares the configured max_size.
        const auto buffer = value_.buffer->prepare(size);
        serializer_.next(ec, [&](boost_code& code, auto const& buffers) NOEXCEPT
        {
            const auto copied = boost::asio::buffer_copy(buffer, buffers);

            // No progress (edge case).
            if (is_zero(copied) && !serializer_.is_done())
            {
                code = to_http_code(http_error_t::unexpected_body);
                return;
            }

            value_.buffer->commit(copied);
            value_.buffer->consume(copied);
        });

        if (ec) return {};
        const auto more = !serializer_.is_done();
        return out_buffer{ std::make_pair(value_.buffer->data(), more) };
    }
    catch (...)
    {
        // As a catch-all we blame alloc.
        ec = to_http_code(http_error_t::bad_alloc);
        return {};
    }
}

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
