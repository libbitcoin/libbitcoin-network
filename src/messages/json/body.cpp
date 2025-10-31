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
#include <bitcoin/network/messages/json/body.hpp>

#include <memory>
#include <utility>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace json {

using namespace system;
using namespace network::error;

// json::body::reader
// ----------------------------------------------------------------------------

void body::reader::init(const http::length_type& length,
    boost_code& ec) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto value = length.get_value_or(zero);
    BC_POP_WARNING()

    if (is_limited<size_t>(value))
    {
        ec = to_http_code(http_error_t::buffer_overflow);
        return;
    }

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    expected_ = possible_narrow_cast<size_t>(value);
    BC_POP_WARNING()

    parser_.reset();
    total_ = zero;
    ec.clear();
}

size_t body::reader::put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
{
    try
    {
        const auto data = pointer_cast<const char>(buffer.data());
        const auto parsed = parser_.write_some(data, buffer.size(), ec);

        total_ = ceilinged_add(total_, parsed);
        if (!ec && total_ > expected_.value_or(max_size_t))
            ec = to_http_code(http_error_t::body_limit);

        return parsed;
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

    return {};
}

void body::reader::finish(boost_code& ec) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    parser_.finish(ec);
    BC_POP_WARNING()

    if (ec) return;

    try
    {
       value_.model = parser_.release();
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

    parser_.reset();
}

// json::body::writer
// ----------------------------------------------------------------------------

void body::writer::init(boost_code& ec) NOEXCEPT
{
    if (!value_.buffer)
    {
        // Caller controls max_size and other buffer behavior by assigning it.
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        value_.buffer = std::make_shared<http::flat_buffer>(default_buffer);
        BC_POP_WARNING()
    }
    else
    {
        // Caller has assigned the buffer (or just reused the response).
        value_.buffer->consume(value_.buffer->size());
    }

    ec.clear();
    serializer_.reset(&value_.model);
}

body::writer::out_buffer body::writer::get(boost_code& ec) NOEXCEPT
{
    ec.clear();
    if (serializer_.done())
        return {};

    const auto size = value_.buffer->max_size();
    if (is_zero(size))
    {
        ec = to_http_code(http_error_t::buffer_overflow);
        return {};
    }

    try
    {
        // Always prepares the configured max_size.
        const auto buff = value_.buffer->prepare(size);
        const auto data = pointer_cast<char>(buff.data());
        const auto view = serializer_.read(data, buff.size());

        // No progress (edge case).
        if (view.empty() && !serializer_.done())
        {
            ec = to_http_code(http_error_t::unexpected_body);
            return {};
        }

        value_.buffer->commit(view.size());
        value_.buffer->consume(view.size());
        const auto more = !serializer_.done();
        return out_buffer{ std::make_pair(boost::asio::buffer(view), more) };
    }
    catch (...)
    {
        // As a catch-all we blame alloc.
        ec = to_http_code(http_error_t::bad_alloc);
        return {};
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin
