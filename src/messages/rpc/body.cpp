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
#include <bitcoin/network/messages/rpc/body.hpp>

#include <memory>
#include <utility>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

using namespace system;
using namespace network::error;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_UNGUARDED_POINTERS)
BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)

// rpc::body::reader
// ----------------------------------------------------------------------------

size_t body::reader::put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
{
    const auto size = buffer.size();
    if (is_zero(size))
    {
        ec.clear();
        return {};
    }

    if (is_null(buffer.data()))
    {
        ec = to_boost_code(boost_error_t::bad_address);
        return {};
    }

    const auto parsed = json::body::reader::put(buffer, ec);
    if (ec || !parser_.done())
        return parsed;

    // http json does not use termination.
    has_terminator_ = false;
    if (!terminated_)
        return parsed;

    // There is no terminator.
    if (is_zero(parsed))
    {
        ec = to_http_code(http_error_t::end_of_stream);
        return parsed;
    }

    // There are extra characters.
    if (size > parsed)
    {
        ec = to_http_code(http_error_t::unexpected_body);
        return parsed;
    }

    // boost::json consumes whitespace, so terminator is always last.
    const auto data = pointer_cast<const char>(buffer.data());
    if (data[sub1(parsed)] == '\n')
    {
        has_terminator_ = true;
        return parsed;
    }

    // There is no terminator.
    ec = to_http_code(http_error_t::end_of_stream);
    return parsed;
}

void body::reader::finish(boost_code& ec) NOEXCEPT
{
    json::body::reader::finish(ec);
    if (ec) return;

    if (terminated_ && !has_terminator_)
    {
        ec = to_http_code(http_error_t::end_of_stream);
        return;
    }

    auto& derived = static_cast<body::value_type&>(value_);
    try
    {
        derived.request = value_to<rpc::request_t>(derived.model);
        derived.model.emplace_null();
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

bool body::reader::is_done() const NOEXCEPT
{
    return parser_.done() && has_terminator_;
}

// rpc::body::writer
// ----------------------------------------------------------------------------

void body::writer::init(boost_code& ec) NOEXCEPT
{
    json::body::writer::init(ec);
    if (ec) return;

    auto& derived = static_cast<body::value_type&>(value_);
    try
    {
        boost::json::value_from(derived.response, value_.model);
    }
    catch (const boost::system::system_error& e)
    {
        // Primary exception type for parsing operations.
        ec = e.code();
        return;
    }
    catch (...)
    {
        // As a catch-all we blame alloc.
        ec = to_http_code(http_error_t::bad_alloc);
        return;
    }

    serializer_.reset(&value_.model);
}

body::writer::out_buffer body::writer::get(boost_code& ec) NOEXCEPT
{
    auto out = json::body::writer::get(ec);
    if (ec || !terminate_) return out;

    constexpr char more = true;
    if (out)
    {
        out->second = more;
        return out;
    }

    using namespace boost::asio;
    static const auto line = '\n';
    return out_buffer{ std::make_pair(buffer(&line, sizeof(line)), !more) };
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace rpc
} // namespace network
} // namespace libbitcoin
