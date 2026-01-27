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
#include <bitcoin/network/net/socket.hpp>

#include <utility>
#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {
    
using namespace system;
using namespace network::rpc;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

/// RPC Read.
// ----------------------------------------------------------------------------

// Buffer could be passed via request, so this is for interface consistency.
void socket::rpc_read(http::flat_buffer& buffer, rpc::request& request,
    count_handler&& handler) NOEXCEPT
{
    boost_code ec{};
    const auto in = emplace_shared<read_rpc>(request);
    in->value.buffer = emplace_shared<http::flat_buffer>(buffer);
    in->reader.init({}, ec);

    boost::asio::dispatch(strand_,
        std::bind(&socket::do_rpc_read,
            shared_from_this(), ec, zero, in, std::move(handler)));
}

void socket::do_rpc_read(boost_code ec, size_t total, const read_rpc::ptr& in,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    constexpr auto size = rpc::writer::default_buffer;

    if (ec)
    {
        // Json parser emits rpc, http and json codes.
        const auto code = error::rpc_to_error_code(ec);
        if (code == error::unknown) logx("rpc-read", ec);
        handler(code, total);
        return;
    }

    VARIANT_DISPATCH_METHOD(get_tcp(),
        async_read_some(in->value.buffer->prepare(size),
            std::bind(&socket::handle_rpc_read,
                shared_from_this(), _1, _2, total, in, handler)));
}

void socket::handle_rpc_read(boost_code ec, size_t size, size_t total,
    const read_rpc::ptr& in, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, total);
        return;
    }

    if (total > maximum_)
    {
        handler(error::message_overflow, total);
        return;
    }

    if (!ec)
    {
        in->value.buffer->commit(size);
        const auto data = in->value.buffer->data();
        const auto parsed = in->reader.put(data, ec);
        if (!ec)
        {
            in->value.buffer->consume(parsed);
            if (in->reader.done())
            {
                in->reader.finish(ec);
                if (!ec)
                {
                    handler(error::success, total);
                    return;
                }
            }
        }
    }

    do_rpc_read(ec, total, in, handler);
}

/// RPC Write.
// ----------------------------------------------------------------------------

void socket::rpc_write(rpc::response& response,
    count_handler&& handler) NOEXCEPT
{
    boost_code ec{};
    const auto out = emplace_shared<write_rpc>(response);
    out->writer.init(ec);

    // Dispatch success or fail, for handler invoke on strand.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_rpc_write,
            shared_from_this(), ec, zero, out, std::move(handler)));
}

void socket::do_rpc_write(boost_code ec, size_t total,
    const write_rpc::ptr& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto buffer = ec ? write_rpc::out_buffer{} : out->writer.get(ec);
    if (ec)
    {
        // Json serializer emits rpc, http and json codes.
        const auto code = error::rpc_to_error_code(ec);
        if (code == error::unknown) logx("rpc-write", ec);
        handler(code, total);
        return;
    }

    BC_ASSERT(buffer.has_value());
    VARIANT_DISPATCH_METHOD(get_tcp(),
        async_write_some(buffer.value().first,
            std::bind(&socket::handle_rpc_write,
                shared_from_this(), _1, _2, total, out, handler)));
}

void socket::handle_rpc_write(boost_code ec, size_t size, size_t total,
    const write_rpc::ptr& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, total);
        return;
    }

    if (!ec && out->writer.done())
    {
        handler(error::success, total);
        return;
    }

    do_rpc_write(ec, total, out, handler);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
