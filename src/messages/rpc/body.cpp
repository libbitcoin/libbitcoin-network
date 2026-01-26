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
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

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

template <>
size_t body<rpc::request_t>::reader::
put(const buffer_type& buffer, boost_code& ec) NOEXCEPT
{
    const auto size = buffer.size();
    if (is_zero(size))
    {
        ec.clear();
        return {};
    }

    if (is_null(buffer.data()))
    {
        ec = to_system_code(boost_error_t::bad_address);
        return {};
    }

    const auto parsed = base::reader::put(buffer, ec);
    if (ec || !parser_.done())
        return parsed;

    // http json does not use termination.
    has_terminator_ = false;
    if (!terminated_)
        return parsed;

    // There is no terminator (terminal).
    if (is_zero(parsed))
    {
        ec = to_http_code(http_error_t::end_of_stream);
        return parsed;
    }

    // boost::json consumes whitespace, and leaves any subsequent chars
    // unparsed, so terminator must be in the parsed segment of the buffer.
    const auto data = pointer_cast<const char>(buffer.data());
    for (auto index = parsed; !is_zero(index);)
    {
        if (data[--index] == '\n')
        {
            // There may be unparsed characters (ok, next message).
            has_terminator_ = true;
            return parsed;
        }
    }

    // There is no terminator (yet).
    return parsed;
}

template <>
bool body<rpc::request_t>::reader::
done() const NOEXCEPT
{
    // Parser may be done but with terminator still outstanding.
    return parser_.done() && (!terminated_ || has_terminator_);
}

template <>
void body<rpc::request_t>::reader::
finish(boost_code& ec) NOEXCEPT
{
    base::reader::finish(ec);
    if (ec) return;

    try
    {
        value_.message = value_to<rpc::request_t>(value_.model);
        value_.model.emplace_null();
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

    // Post-parse semantic validation.

    if (value_.message.jsonrpc == version::undefined)
        value_.message.jsonrpc = version::v1;

    if (value_.message.method.empty() ||
        !value_.message.params.has_value())
    {
        ec = to_system_code(boost_error_t::bad_message);
        return;
    }

    if (value_.message.jsonrpc == version::v1)
    {
        if (!value_.message.id.has_value())
            ec = to_system_code(boost_error_t::bad_message);
        else if (!std::holds_alternative<array_t>(
            value_.message.params.value()))
            ec = to_system_code(boost_error_t::bad_message);

        // TODO: v1 batch is not allowed.
        ////else if (value_.message.is_batch())
        ////    ec = to_system_code(boost_error_t::bad_message);
    }
}

template <>
size_t body<rpc::response_t>::reader::
put(const buffer_type&, boost_code&) NOEXCEPT
{
    BC_ASSERT(false);
    return {};
}

template <>
void body<rpc::response_t>::reader::
finish(boost_code&) NOEXCEPT
{
    BC_ASSERT(false);
}

template <>
bool body<rpc::response_t>::reader::
done() const NOEXCEPT
{
    BC_ASSERT(false);
    return {};
}

// rpc::body::writer
// ----------------------------------------------------------------------------

template <>
void body<rpc::response_t>::writer::
init(boost_code& ec) NOEXCEPT
{
    base::writer::init(ec);
    if (ec) return;

    try
    {
        boost::json::value_from(value_.message, value_.model);
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

    set_terminator_ = false;
    serializer_.reset(&value_.model);
}

template <>
body<rpc::response_t>::writer::out_buffer
body<rpc::response_t>::writer::
get(boost_code& ec) NOEXCEPT
{
    auto out = base::writer::done() ? out_buffer{} : base::writer::get(ec);
    if (ec) return out;

    // Override json reader !more so terminator can be added.
    if (out.has_value())
    {
        out.value().second = true;
        return out;
    }

    // Add terminator and signal done.
    set_terminator_ = true;
    using namespace boost::asio;
    static constexpr auto line = '\n';
    return out_buffer{ std::make_pair(buffer(&line, sizeof(line)), false) };
}

template <>
bool body<rpc::response_t>::writer::
done() const NOEXCEPT
{
    // Done is redundant with !out.second, but provides a cleaner interface.
    return base::writer::done() && (!terminate_ || set_terminator_);
}

template <>
void body<rpc::request_t>::writer::
init(boost_code&) NOEXCEPT
{
    BC_ASSERT(false);
}

template <>
body<rpc::request_t>::writer::out_buffer
body<rpc::request_t>::writer::
get(boost_code&) NOEXCEPT
{
    BC_ASSERT(false);
    return {};
}

template <>
bool body<rpc::request_t>::writer::
done() const NOEXCEPT
{
    BC_ASSERT(false);
    return {};
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace rpc
} // namespace network
} // namespace libbitcoin
