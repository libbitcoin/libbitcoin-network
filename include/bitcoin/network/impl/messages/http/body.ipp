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
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

// reader
// ----------------------------------------------------------------------------

template <bool IsRequest, class Fields>
inline body::reader::reader(header<IsRequest, Fields>& ,
    value_type& ) NOEXCEPT
  ////: payload_{ payload }
{
}

inline void body::reader::init(const length_type& ,
    error_code& ) NOEXCEPT
{
}

inline size_t body::reader::put(const buffer_type& ,
    error_code& ) NOEXCEPT
{
    return{};
}

inline void body::reader::finish(error_code& ) NOEXCEPT
{
}

// writer
// ----------------------------------------------------------------------------

template <bool IsRequest, class Fields>
inline body::writer::writer(header<IsRequest, Fields>& ,
    const value_type& ) NOEXCEPT
  ////: payload_{ payload }
{
}

inline void body::writer::init(error_code& ) NOEXCEPT
{
}

inline body::writer::out_buffer body::writer::get(error_code& ) NOEXCEPT
{
    return{};
}

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
