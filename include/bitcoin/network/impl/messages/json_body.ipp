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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_IPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_IPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace json {

// json::body<>::reader
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::reader::init(const http::length_type& length,
    code& ec) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto value = length.value_or(max_size_t);
    BC_POP_WARNING()
        
    using namespace system;
    if (is_limited<size_t>(value))
    {
        using namespace network::error;
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

TEMPLATE
size_t CLASS::reader::put(const buffer_type& buffer, code& ec) NOEXCEPT
{
    using namespace system;
    using namespace network::error;

    const auto size = buffer.size();
    if (is_zero(size))
    {
        ec.clear();
        return {};
    }

    if (is_null(buffer.data()))
    {
        ec = to_http_code(http_error_t::bad_alloc);
        return {};
    }

    try
    {
        const auto data = pointer_cast<const char>(buffer.data());
        size_t parsed{};
        ec.clear();

        // The parser stops after the document, the remainder is re-fed.
        if (parser_.done())
        {
            while (parsed < size && is_json_whitespace(data[parsed]))
                ++parsed;

            if (parsed < size)
                ec = to_json_code(json_error_t::json_extra_data);
        }
        else
        {
            code error{};
            parsed = parser_.write_some(data, size, error);
            if (error)
                ec = error;
        }

        total_ = ceilinged_add(total_, parsed);
        if (!ec && total_ > expected_.value_or(max_size_t))
            ec = to_http_code(http_error_t::body_limit);

        return parsed;
    }
    catch (...)
    {
        // As a catch-all we blame alloc.
        ec = to_http_code(http_error_t::bad_alloc);
    }

    return {};
}

// Finishing can be confusing. Derived rpc body calls this base method,
// then virtual done() is tested to determine whether logical object is
// fully read (including optionally required terminator). Any error code
// here signals http parser (and any reader that terminates on end of
// framed data, such as websockets) that parse has failed (terminal
// error). However for custom stream readers that may not be aware of byte
// termination, `need_more` implies parse has not failed and more bytes
// may be parsed. In either case, when called and parse is complete,
// parsed json object is moved to model and parser is released. In derived
// json-rpc reader, json is then also converted to rpc model if valid,
// otherwise returning failure code. Underlying parser_.finish(ec) is
// never called, as that would preclude use in unbounded scenario.
TEMPLATE
void CLASS::reader::finish(code& ec) NOEXCEPT
{
    using namespace network::error;

    // Parser always returns !done() when parsing top-level number as whole
    // document, since it cannot know if more digits are coming. So this returns
    // need_more even when fixed-size buffer is being read. For this reason this
    // body does not support *reading* top-level numbers.
    if (!done())
    {
        ec = to_http_code(http_error_t::need_more);
        return;
    }

    ec.clear();
    value_.model = parser_.release();
}

TEMPLATE
bool CLASS::reader::done() const NOEXCEPT
{
    return parser_.done();
}

// json::body<>::writer
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::writer::init(code& ec) NOEXCEPT
{
    using namespace system;
    using namespace network::error;

    // The buffer is retained by the socket, which bounds its allocation.
    if (is_null(value_.buffer))
    {
        ec = to_http_code(http_error_t::bad_alloc);
        return;
    }

    // Serialization is chunked by the estimate, within that bound.
    chunk_ = is_zero(value_.size_hint) ? max_size_t : value_.size_hint;
    ec.clear();
    serializer_.reset(&value_.model);
}

TEMPLATE
CLASS::writer::out_buffer
CLASS::writer::get(code& ec) NOEXCEPT
{
    using namespace network::error;
    if (done())
    {
        ec = to_http_code(http_error_t::end_of_stream);
        return {};
    }

    if (!value_.buffer)
    {
        ec = to_http_code(http_error_t::bad_alloc);
        return {};
    }

    // Chunk and any terminator, which follows final serialized byte, are bound
    // by room following buffer's readable bytes.
    const auto reserve = is_zero(terminator_) ? zero : one;
    const auto size = std::min(chunk_, system::floored_subtract(
        value_.buffer->max_size(), system::ceilinged_add(
            value_.buffer->size(), reserve)));

    if (is_zero(size))
    {
        ec = to_http_code(http_error_t::buffer_overflow);
        return {};
    }

    try
    {
        const auto scratch = value_.buffer->prepare(size + reserve);
        const auto data = system::pointer_cast<char>(scratch.data());
        const auto view = serializer_.read(data, size);

        // No progress (edge case).
        if (view.empty() && !serializer_.done())
        {
            ec = to_http_code(http_error_t::unexpected_body);
            return {};
        }

        ec.clear();
        const auto more = !serializer_.done();
        if (more || is_zero(terminator_))
            return out_buffer{ std::make_pair(io::buffer(view), more) };

        data[view.size()] = terminator_;
        const auto bytes = add1(view.size());
        return out_buffer{ std::make_pair(io::buffer(data, bytes), more) };
    }
    catch (...)
    {
        // As a catch-all we blame alloc.
        ec = to_http_code(http_error_t::bad_alloc);
    }

    return {};
}

TEMPLATE
bool CLASS::writer::done() const NOEXCEPT
{
    return serializer_.done();
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
