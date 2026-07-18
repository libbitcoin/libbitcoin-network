/**
 * Copyright (c) 2011-2026 libbitcoin developers (see AUTHORS)
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

// BODY (read).
// ----------------------------------------------------------------------------
// The body_ methods accept any boost::beast-compliant body to read/write a tcp
// socket. For http read/write use the http_ methods. The rpc_ methods are just
// a specialization of these methods, passing the rpc::body<> types. For simple
// fixed-size tcp (p2p) use the tcp_ methods, and for simple framed ws use the
// ws_ methods. The body methods require fixed size or framed read/write. The
// json and json-rpc bodies are internally framed by json, so can read/write
// over a raw tcp socket (electrum) or a framed websocket (btcd). And again for
// http framing, use the http_ methods, as these incorporate header processing.

void socket::body_read(http::flat_buffer& buffer, http::request& request,
    count_handler&& handler) NOEXCEPT
{
    boost_code ec{};
    const auto in = emplace_shared<read_state>(request, buffer);
    in->reader.init({}, ec);

    boost::asio::dispatch(strand_,
        std::bind(&socket::do_body_read,
            shared_from_this(), ec, zero, in, std::move(handler)));
}

// private
void socket::do_body_read(boost_code ec, size_t total,
    const read_state::ptr& in, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec)
    {
        const auto code = error::http_to_error_code(ec);
        if (code == error::unknown) logx("body-read", ec);
        handler(code, total);
        return;
    }

    // Parse data in the buffer.
    const auto read = in->reader.put(in->buffer.data(), ec);

    // Batchable bodies deliver each message by pausing the parse, with
    // residue carried in the buffer to the next logical read.
    if (error::http_to_error_code(ec) == error::need_buffer)
    {
        in->buffer.consume(read);
        handler(error::success, total);
        return;
    }

    if (!ec && !is_zero(read))
    {
        in->buffer.consume(read);

        // The json/json-rpc readers do not finalize on finish, instead
        // they return need_more if not complete and success if complete.
        // For others this assumes websockets, since finish is called
        // after the first read (full message must be complete).
        in->reader.finish(ec);

        if (!ec)
        {
            handler(error::success, total);
            return;
        }

        if (ec == error::http_error_t::need_more)
            ec.clear();
    }

    if (ec)
    {
        const auto code = error::http_to_error_code(ec);
        if (code == error::unknown) logx("body-read", ec);
        handler(code, total);
        return;
    }

    if (total > maximum_)
    {
        handler(error::message_overflow, total);
        return;
    }

    // This reader is designed to accept native beast bodies, which require the
    // buffer size to be known so that finish() is called only after all data
    // is passed. For websockets this reads a full logical message into the
    // flat buffer. For tcp the reader will iterate over the buffer using
    // async_read_some here, and calls reader.finish() after each. So the body
    // reader is usable on tcp only for bodies that allow finish() iteration.
    async_read(in->buffer,
        std::bind(&socket::handle_body_read,
            shared_from_this(), _1, _2, total, in, handler));
}

// private
void socket::handle_body_read(const code& ec, size_t size, size_t total,
    const read_state::ptr& in, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (ec == error::operation_canceled)
    {
        handler(error::channel_stopped, total);
        return;
    }

    if (total > maximum_)
    {
        handler(error::message_overflow, total);
        return;
    }

    if (ec)
    {
        handler(ec, total);
        return;
    }

    // Beast websocket read commits framed message.
    if (!websocket())
        in->buffer.commit(size);

    do_body_read({}, total, in, handler);
}

// Body (write).
// ----------------------------------------------------------------------------

void socket::body_write(http::response&& response,
    count_handler&& handler) NOEXCEPT
{
    boost_code ec{};
    const auto out = emplace_shared<write_state>(std::move(response));
    out->writer.init(ec);

    boost::asio::dispatch(strand_,
        std::bind(&socket::do_body_write,
            shared_from_this(), ec, zero, out, std::move(handler)));
}

// private
void socket::do_body_write(boost_code ec, size_t total,
    const write_state::ptr& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto buffer = ec ? write_state::out_buffer{} : out->writer.get(ec);
    if (!buffer.has_value())
    {
        handler(error::bad_stream, total);
        return;
    }

    if (ec)
    {
        const auto code = error::http_to_error_code(ec);
        if (code == error::unknown) logx("body-write", ec);
        handler(code, total);
        return;
    }

    out->more = buffer.value().second;
    const auto& data = buffer.value().first;

    async_write(data, out->writer.binary(),
        std::bind(&socket::handle_body_write,
            shared_from_this(), _1, _2, total, out, handler));
}

// private
void socket::handle_body_write(const code& ec, size_t size, size_t total,
    const write_state::ptr& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (ec == error::operation_canceled)
    {
        handler(error::channel_stopped, total);
        return;
    }

    if (!ec && !out->more)
    {
        handler(error::success, total);
        return;
    }

    if (ec)
    {
        handler(ec, total);
        return;
    }

    // Handle error condition or incomplete message.
    do_body_write({}, total, out, handler);
}

// Body (notify).
// ----------------------------------------------------------------------------
// Identical to body_write() apart from request vs. response.

void socket::body_notify(http::request&& notification,
    count_handler&& handler) NOEXCEPT
{
    boost_code ec{};
    const auto out = emplace_shared<notify_state>(std::move(notification));
    out->writer.init(ec);

    boost::asio::dispatch(strand_,
        std::bind(&socket::do_body_notify,
            shared_from_this(), ec, zero, out, std::move(handler)));
}

// private
void socket::do_body_notify(boost_code ec, size_t total,
    const notify_state::ptr& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    const auto buffer = ec ? notify_state::out_buffer{} : out->writer.get(ec);
    if (!buffer.has_value())
    {
        handler(error::bad_stream, total);
        return;
    }

    if (ec)
    {
        const auto code = error::http_to_error_code(ec);
        if (code == error::unknown) logx("body-notify", ec);
        handler(code, total);
        return;
    }

    out->more = buffer.value().second;
    const auto& data = buffer.value().first;

    // TODO: derive websocket binary/text from body type mapping.
    async_write(data, false,
        std::bind(&socket::handle_body_notify,
            shared_from_this(), _1, _2, total, out, handler));
}

// private
void socket::handle_body_notify(const code& ec, size_t size, size_t total,
    const notify_state::ptr& out, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    total = ceilinged_add(total, size);
    if (ec == error::operation_canceled)
    {
        handler(error::channel_stopped, total);
        return;
    }

    if (!ec && !out->more)
    {
        handler(error::success, total);
        return;
    }

    if (ec)
    {
        handler(ec, total);
        return;
    }

    // Handle error condition or incomplete message.
    do_body_notify({}, total, out, handler);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
