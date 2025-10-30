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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_IPP

#include <memory>
#include <utility>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

// reader
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::reader::init(const http::length_type& length,
    error_code& ec) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto value = length.get_value_or(zero);
    BC_POP_WARNING()

    if (system::is_limited<size_t>(value))
    {
        ec = error::to_boost_code(error::boost_error_t::protocol_error);
        return;
    }

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    expected_ = system::possible_narrow_cast<size_t>(value);
    BC_POP_WARNING()

    parser_.reset();
    total_ = zero;
    ec.clear();
}

TEMPLATE
size_t CLASS::reader::put(const buffer_type& buffer, error_code& ec) NOEXCEPT
{
    try
    {
        const auto data = system::pointer_cast<const char>(buffer.data());
        const size_t parsed = parser_.write_some(data, buffer.size(), ec);

        total_ = system::ceilinged_add(total_, parsed);
        if (!ec && total_ > expected_.value_or(max_size_t))
            ec = error::to_boost_code(error::boost_error_t::protocol_error);

        return parsed;
    }
    catch (...)
    {
        ec = error::to_boost_code(error::boost_error_t::protocol_error);
        return {};
    }
}

TEMPLATE
void CLASS::reader::finish(error_code& ec) NOEXCEPT
{
    try
    {
        parser_.finish(ec);

        if (!ec)
            value_.model = parser_.release();
    }
    catch (...)
    {
        ec = error::to_boost_code(error::boost_error_t::protocol_error);
    }
}

// writer
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::writer::init(error_code& ec) NOEXCEPT
{
    if (!value_.buffer)
    {
        // Caller controls max_size and other buffer behavior by assigning it.
        value_.buffer = std::make_shared<http::flat_buffer>(default_buffer);
    }
    else
    {
        // Caller has assigned the buffer (or just reused the response).
        value_.buffer->consume(value_.buffer->size());
    }

    ec.clear();
    serializer_.reset(&value_.model);
}

TEMPLATE
CLASS::writer::out_buffer CLASS::writer::get(error_code& ec) NOEXCEPT
{
    ec.clear();
    if (serializer_.done())
        return {};

    try
    {
        // Always prepares the configured max_size.
        const auto size = value_.buffer->max_size();
        const auto buff = value_.buffer->prepare(size);
        const auto data = system::pointer_cast<char>(buff.data());
        const auto view = serializer_.read(data, buff.size());
        const auto done = serializer_.done();
        value_.buffer->commit(view.size());
        return out_buffer{ std::make_pair(boost::asio::buffer(view), !done) };
    }
    catch (...)
    {
        ec = error::to_boost_code(error::boost_error_t::protocol_error);
        return {};
    }
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
