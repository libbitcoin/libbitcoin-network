/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include <bitcoin/network/messages/http_body.hpp>

#include <variant>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace network::error;
    
// http::body::reader
// ----------------------------------------------------------------------------

void body::reader::init(const length_type& length, boost_code& ec) NOEXCEPT
{
    // Header is unread at construct, so this must be deferred until init.
    assign_reader(header_, value_);

    std::visit(overload
    {
        [&](std::monostate&) NOEXCEPT
        {
            ec = to_http_code(http_error_t::end_of_stream);
        },
        [&](auto& read) NOEXCEPT
        {
            try
            {
                read.init(length, ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
            }
        }
    }, reader_);
}

size_t body::reader::put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
{
    return std::visit(overload
    {
        [&](std::monostate&) NOEXCEPT
        {
            ec = to_http_code(http_error_t::end_of_stream);
            return size_t{};
        },
        [&](auto& read) NOEXCEPT
        {
            try
            {
                return read.put(buffer, ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
                return size_t{};
            }
        }
    }, reader_);
}

void body::reader::finish(boost_code& ec) NOEXCEPT
{
    std::visit(overload
    {
        [&](std::monostate&) NOEXCEPT
        {
            // Called at beast finish_header and must succeed.
            ec = {};
        },
        [&](auto& read) NOEXCEPT
        {
            try
            {
                read.finish(ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
            }
        }
    }, reader_);
}

// http::body::writer
// ----------------------------------------------------------------------------
    
void body::writer::init(boost_code& ec) NOEXCEPT
{
    std::visit(overload
    {
        [&] (std::monostate&) NOEXCEPT
        {
            ec = {};
        },
        [&](auto& write) NOEXCEPT
        {
            try
            {
                write.init(ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
            }
        }
    }, writer_);
}

body::writer::out_buffer body::writer::get(boost_code& ec) NOEXCEPT
{
    return std::visit(overload
    {
        [&] (std::monostate&) NOEXCEPT
        {
            ec = {};
            return out_buffer{};
        },
        [&](empty_writer&) NOEXCEPT
        {
            ec = {};

            // Socket body writer requires non-empty buffer to write empty.
            return out_buffer{ std::make_pair(asio::const_buffer{}, false) };
        },
        [&](auto& write) NOEXCEPT
        {
            try
            {
                return write.get(ec);
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::end_of_stream);
                return out_buffer{};
            }
        }
    }, writer_);
}

// http::body::streaming
// ----------------------------------------------------------------------------

bool body::streaming(const value_type& value) NOEXCEPT
{
    // A buffer body emits an unbounded sequence of caller-supplied buffers
    // while more is set, so its total length is not knowable here.
    return value.contains<buffer_value>() && value.get<buffer_value>().more;
}

// http::body::unframable_zero
// ----------------------------------------------------------------------------

bool body::unframable_zero(const value_type& value) NOEXCEPT
{
    return value.contains<json_value>() || value.contains<rpc::request>() ||
        value.contains<rpc::response>() || value.contains<peer_value>();
}

// http::body::size
// ----------------------------------------------------------------------------
// A length cannot be produced without serializing the whole model, so
// measuring json forfeits the laziness that the streaming writer exists for.
// The writer still streams, but it streams bytes that were measured in full
// before the header was written. bitcoind declares a length on every
// response, so an interface that implements it has no alternative.

uint64_t body::size(const value_type& value) NOEXCEPT
{
    using namespace system;

    // The writer assigns an empty body when the caller has assigned none.
    if (!value.has_value())
        return zero;

    return std::visit(overload
    {
        [](const empty_value& value) NOEXCEPT
        {
            return empty_body::size(value);
        },
        [](const data_value& value) NOEXCEPT
        {
            return chunk_body::size(value);
        },
        [](const file_value& value) NOEXCEPT
        {
            return file_body::size(value);
        },
        [](const span_value& value) NOEXCEPT
        {
            return span_body::size(value);
        },
        [](const buffer_value& value) NOEXCEPT -> uint64_t
        {
            // The writer emits the assigned buffer, which is the whole body
            // only where more is unset (a streaming body is not framable).
            return is_null(value.data) ? zero : value.size;
        },
        [](const string_value& value) NOEXCEPT
        {
            return string_body::size(value);
        },
        [](const json_value& value) NOEXCEPT
        {
            return json_body::length(value.model);
        },
        [](const peer_value&) NOEXCEPT -> uint64_t
        {
            // A peer body is selected only by preselection (non-http) and is
            // written by proxy::write(frame&&), so it is never framed by
            // beast and this measure is never obtained.
            return zero;
        },
        [](const rpc::request& value) NOEXCEPT
        {
            return rpc::request_body::size(value);
        },
        [](const rpc::response& value) NOEXCEPT
        {
            return rpc::response_body::size(value);
        }
    }, value.value());
}

} // namespace http
} // namespace network
} // namespace libbitcoin
