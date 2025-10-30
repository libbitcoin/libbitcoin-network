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
#include <bitcoin/network/messages/http/body.hpp>

#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace network::error;
    
// http::body::reader
// ----------------------------------------------------------------------------

void body::reader::init(const length_type& length,
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
                ec = to_boost_code(boost_error_t::io_error);
            }
        }
    }, reader_);
}

size_t body::reader::put(const buffer_type& buffer,
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
                ec = to_boost_code(boost_error_t::io_error);
                return size_t{};
            }
        }
    }, reader_);
}

void body::reader::finish(error_code& ec) NOEXCEPT
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
                ec = to_boost_code(boost_error_t::io_error);
            }
        }
    }, reader_);
}

// http::body::writer
// ----------------------------------------------------------------------------
    
void body::writer::init(error_code& ec) NOEXCEPT
{
    return std::visit(overload
    {
        [&](auto& write) NOEXCEPT
        {
            try
            {
                write.init(ec);
            }
            catch (...)
            {
                ec = to_boost_code(boost_error_t::io_error);
            }
        }
    }, writer_);
}

body::writer::out_buffer body::writer::get(error_code& ec) NOEXCEPT
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
                ec = to_boost_code(boost_error_t::io_error);
                return out_buffer{};
            }
        }
    }, writer_);
}

} // namespace http
} // namespace network
} // namespace libbitcoin
