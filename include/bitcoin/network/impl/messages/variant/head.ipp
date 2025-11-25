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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_VARIANT_HEAD_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_VARIANT_HEAD_IPP

#include <variant>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace variant {
    
// variant::head::reader
// ----------------------------------------------------------------------------

TEMPLATE
inline void CLASS::reader::init(const http::length_type& length,
    boost_code& ec) NOEXCEPT
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

TEMPLATE
inline size_t CLASS::reader::put(const buffer_type& buffer,
    boost_code& ec) NOEXCEPT
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

TEMPLATE
inline void CLASS::reader::finish(boost_code& ec) NOEXCEPT
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

// variant::head::writer
// ----------------------------------------------------------------------------

TEMPLATE
inline void CLASS::writer::init(boost_code& ec) NOEXCEPT
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
                ec = error::to_boost_code(error::boost_error_t::io_error);
            }
        }
    }, writer_);
}

TEMPLATE
inline CLASS::writer::out_buffer CLASS::writer::get(boost_code& ec) NOEXCEPT
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

} // namespace variant
} // namespace network
} // namespace libbitcoin

#endif
