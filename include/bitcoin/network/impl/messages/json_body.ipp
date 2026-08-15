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

#include <array>
#include <memory>
#include <utility>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace json {

// json::body<>::reader
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::reader::init(const http::length_type& length,
    boost_code& ec) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    const auto value = length.get_value_or(max_size_t);
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
size_t CLASS::reader::put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
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

// Finishing can be very confusing. The derived rpc body calls this base
// method, and then the virtual done() is tested to determine whether the
// logical object is fully read (including optionally required terminator).
// Any error code here signals the beast reader (and any reader that terminates
// based on the end of the framed data, such as websockets) that the parse has
// failed (terminal error). However for custom stream readers that may not be
// aware of byte termination, the `need_more` implies that the parse has not
// failed and that more bytes may be parsed. In either case, when this is
// called and the parse is complete, then the parsed json object is moved to
// the model and the parser is released. In the case of the derived json-rpc
// reader, the json is also then converted to the rpc model if valid, otherwise
// returning a failure code. In no case is the underlying parser_.finish(ec)
// ever called, as that would preclude use in the unbounded scenario.
TEMPLATE
void CLASS::reader::finish(boost_code& ec) NOEXCEPT
{
    using namespace network::error;

    // The internal boost::json parser will always return !done() when parsing
    // a top-level primitive as a whole document, specifically a number number
    // value, because it has no way to know if more digits are coming. So this
    // will return need_more even when a fixed-size buffer is being read. For
    // this reason this body does not support *reading* for top-level objects.
    if (!done())
    {
        ec = to_http_code(http_error_t::need_more);
        return;
    }

    try
    {
        ec.clear();
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

TEMPLATE
bool CLASS::reader::done() const NOEXCEPT
{
    return parser_.done();
}

// json::body<>::writer
// ----------------------------------------------------------------------------

TEMPLATE
void CLASS::writer::init(boost_code& ec) NOEXCEPT
{
    using namespace system;
    const auto size = is_zero(value_.size_hint) ? default_buffer :
        value_.size_hint;

    if (!value_.buffer)
    {
        value_.buffer = emplace_shared<http::flat_buffer>(size);
    }
    else
    {
        // Caller has assigned the buffer (or just reused the response).
        // In a full duplex channel the buffer cannot be modified by caller
        // even from the strand, due to write interleaving.
        value_.buffer->consume(value_.buffer->size());
        value_.buffer->max_size(size);
    }

    ec.clear();
    serializer_.reset(&value_.model);
}

TEMPLATE
CLASS::writer::out_buffer
CLASS::writer::get(boost_code& ec) NOEXCEPT
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

    const auto size = value_.buffer->max_size();
    if (is_zero(size))
    {
        ec = to_http_code(http_error_t::buffer_overflow);
        return {};
    }

    try
    {
        // Always prepares the configured max_size.
        const auto scratch = value_.buffer->prepare(size);
        const auto data = system::pointer_cast<char>(scratch.data());
        const auto view = serializer_.read(data, scratch.size());

        // No progress (edge case).
        if (view.empty() && !serializer_.done())
        {
            ec = to_http_code(http_error_t::unexpected_body);
            return {};
        }

        ec.clear();
        value_.buffer->consume(scratch.size());
        const auto more = !serializer_.done();
        return out_buffer{ std::make_pair(boost::asio::buffer(view), more) };
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

TEMPLATE
bool CLASS::writer::done() const NOEXCEPT
{
    return serializer_.done();
}

// json::body<>::length
// ----------------------------------------------------------------------------

TEMPLATE
uint64_t CLASS::length(const boost::json::value& model) NOEXCEPT
{
    using namespace system;

    // The scratch is overwritten by each read, so the serialization is
    // measured without being materialized. The alternative is not a cheaper
    // measure but no measure, which frames the response as chunked.
    std::array<char, writer::default_buffer> scratch{};
    size_t size{};

    try
    {
        // The writer serializes the same model with the same configuration,
        // so a model that cannot be measured here cannot be written there.
        boost::json::serializer serializer{ model.storage() };
        serializer.reset(&model);

        while (!serializer.done())
        {
            const auto view = serializer.read(scratch.data(), scratch.size());

            // No progress (edge case), as guarded by the writer.
            if (view.empty())
                return zero;

            size = ceilinged_add(size, view.size());
        }
    }
    catch (...)
    {
        // The writer performs this same serialization, so it fails likewise.
        return zero;
    }

    return size;
}

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
