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
#include <bitcoin/network/net/socket.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/deadline.hpp>

// Async operations are not thread safe for the same socket object.
// Calls are stranded to protect the socket member.

namespace libbitcoin {
namespace network {

using namespace system;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Bounds query overrun following caller drop, not response latency.
constexpr auto watch_interval = milliseconds(10);

// Construct.
// ----------------------------------------------------------------------------

socket::socket(const logger& log, io::context& service,
    const parameters& parameters, bool proxied) NOEXCEPT
  : socket(log, service, parameters, {}, {}, proxied, true)
{
}

socket::socket(const logger& log, io::context& service,
    const parameters& params, const config::address& address,
    const config::endpoint& endpoint, bool proxied) NOEXCEPT
  : socket(log, service, params, address, endpoint, proxied, false)
{
}

// protected
socket::socket(const logger& log, io::context& service,
    const parameters& params, const config::address& address,
    const config::endpoint& endpoint, bool proxied, bool inbound) NOEXCEPT
  : inbound_(inbound),
    proxied_(proxied),
    maximum_(params.maximum_request),
    header_(std::min(params.maximum_request, maximum_http_header)),
    minimum_buffer_(params.minimum_buffer),
    bind_(params.bind),
    slot_(params.slot),
    strand_(service.get_executor()),
    service_(service),
    context_(params.context),
    role_(params.role),
    address_(address),
    endpoint_(endpoint),
    timer_(emplace_shared<deadline>(log, strand_, params.connect_timeout)),
    watch_(emplace_shared<deadline>(log, strand_, watch_interval)),
    socket_(std::in_place_type<io::socket>, strand_),
    response_(params.maximum_buffer),
    reporter(log),
    tracker<socket>(log)
{
}

socket::~socket() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "socket is not stopped");
    if (!stopped_.load()) { LOGF("~socket is not stopped."); }
}

// Wait.
// ----------------------------------------------------------------------------

void socket::watch(result_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_watch,
            shared_from_this(), std::move(handler)));
}

void socket::poll(result_handler&& handler) NOEXCEPT
{
    io::dispatch(strand_,
        std::bind(&socket::do_poll,
            shared_from_this(), std::move(handler)));
}

// private
void socket::do_watch(const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped_.load())
    {
        handler(error::success);
        return;
    }

    watch_->start(std::bind(&socket::handle_watch,
        shared_from_this(), _1, handler));
}

// private
void socket::do_poll(const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (stopped_.load())
    {
        handler(error::success);
        return;
    }

    watch_->start(std::bind(&socket::handle_poll,
        shared_from_this(), _1, handler));
}

// private
void socket::handle_watch(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Only timer stop results in caller not stopping the channel.
    if (ec)
    {
        handler(error::success);
        return;
    }

    if (get_base().half_closed())
    {
        handler(error::peer_disconnect);
        return;
    }

    do_watch(handler);
}

// private
void socket::handle_poll(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Timer stop (socket stop) results in caller not stopping the channel.
    if (!ec && get_base().half_closed())
    {
        handler(error::peer_disconnect);
        return;
    }

    handler(error::success);
}

// Properties.
// ----------------------------------------------------------------------------

io::context& socket::service() const NOEXCEPT
{
    return service_;
}

io::strand& socket::strand() NOEXCEPT
{
    return strand_;
}

bool socket::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}

bool socket::stopped() const NOEXCEPT
{
    return stopped_.load();
}

bool socket::inbound() const NOEXCEPT
{
    return inbound_;
}

bool socket::websocket() const NOEXCEPT
{
    return websocket_.load();
}

bool socket::downgraded() const NOEXCEPT
{
    return downgraded_.load();
}

bool socket::detected() const NOEXCEPT
{
    return detected_.load();
}

const config::address& socket::address() const NOEXCEPT
{
    return address_;
}

const config::endpoint& socket::endpoint() const NOEXCEPT
{
    return endpoint_;
}

const config::endpoint& socket::binding() const NOEXCEPT
{
    return binding_;
}

size_t socket::slot() const NOEXCEPT
{
    return slot_;
}

void socket::set_address(const config::address& address) NOEXCEPT
{
    BC_ASSERT(stranded());
    address_ = address;
    endpoint_ = address;
}

// Variant state (protected by strand).
// ----------------------------------------------------------------------------

bool socket::secure() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<tls::stream>(socket_);
}

bool socket::encrypted() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<p2ps::stream>(socket_);
}

bool socket::zeromq() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<zmtp::stream>(socket_);
}

// protected
bool socket::is_base() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<io::socket>(socket_);
}

// Variant accessors.
// ----------------------------------------------------------------------------
// protected

socket::tcp_t socket::get_tcp() NOEXCEPT
{
    BC_ASSERT(stranded());

    return std::visit(overload
    {
        [](io::socket& value) NOEXCEPT -> socket::tcp_t
        {
            return std::ref(value);
        },
        [](tls::stream& value) NOEXCEPT -> socket::tcp_t
        {
            return std::ref(value);
        },
        [](p2ps::stream&) NOEXCEPT -> socket::tcp_t
        {
            std::terminate();
        },
        [](zmtp::stream&) NOEXCEPT -> socket::tcp_t
        {
            std::terminate();
        }
    }, socket_);
}

io::socket& socket::get_base() NOEXCEPT
{
    return std::visit(overload
    {
        [](io::socket& value) NOEXCEPT -> io::socket&
        {
            return value;
        },
        [](tls::stream& value) NOEXCEPT -> io::socket&
        {
            return value.next_layer();
        },
        [](p2ps::stream& value) NOEXCEPT -> io::socket&
        {
            return value.next_layer();
        },
        [](zmtp::stream& value) NOEXCEPT -> io::socket&
        {
            return value.next_layer();
        }
    }, socket_);
}

tls::stream& socket::get_tls() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(secure());

    return std::visit(overload
    {
        [](io::socket&) NOEXCEPT -> tls::stream&
        {
            std::terminate();
        },
        [](tls::stream& value) NOEXCEPT -> tls::stream&
        {
            return value;
        },
        [](p2ps::stream&) NOEXCEPT -> tls::stream&
        {
            std::terminate();
        },
        [](zmtp::stream&) NOEXCEPT -> tls::stream&
        {
            std::terminate();
        }
    }, socket_);
}

p2ps::stream& socket::get_p2ps() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(encrypted());

    return std::visit(overload
    {
        [](io::socket&) NOEXCEPT -> p2ps::stream&
        {
            std::terminate();
        },
        [](tls::stream&) NOEXCEPT -> p2ps::stream&
        {
            std::terminate();
        },
        [](p2ps::stream& value) NOEXCEPT -> p2ps::stream&
        {
            return value;
        },
        [](zmtp::stream&) NOEXCEPT -> p2ps::stream&
        {
            std::terminate();
        }
    }, socket_);
}

zmtp::stream& socket::get_zmtp() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(zeromq());

    return std::visit(overload
    {
        [](io::socket&) NOEXCEPT -> zmtp::stream&
        {
            std::terminate();
        },
        [](tls::stream&) NOEXCEPT -> zmtp::stream&
        {
            std::terminate();
        },
        [](p2ps::stream&) NOEXCEPT -> zmtp::stream&
        {
            std::terminate();
        },
        [](zmtp::stream& value) NOEXCEPT -> zmtp::stream&
        {
            return value;
        }
    }, socket_);
}

// Variant (ws vs. tcp)  helpers.
// ----------------------------------------------------------------------------
// protected

// write message frame (ws, finish closes the message) or unframed fixed
// size bytes (tcp). The binary/text setting applies only at the start of a
// message, so it is harmless to pass it on every frame of the message.
void socket::async_write(const io::const_buffer& buffer, bool binary,
    bool finish, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (websocket())
    {
        ws_send_data(buffer, binary, finish, handler);
    }
    else
    {
        VARIANT_DISPATCH_FUNCTION(io::async_write, get_tcp(),
            buffer, std::bind(&socket::handle_async,
                shared_from_this(), _1, _2, handler, "async_write"));
    }
}

// read arbitrary tcp bytes, iterate (up to buffer capacity).
void socket::async_read_some(const io::mutable_buffer& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (websocket())
    {
        // Websockets read messages, use async_read(flat_buffer).
        handler(error::operation_failed, {});
    }
    else
    {
        VARIANT_DISPATCH_METHOD(get_tcp(),
            async_read_some(buffer, std::bind(&socket::handle_async,
                shared_from_this(), _1, _2, handler, "async_read_some")));
    }
}

// read arbitrary tcp bytes, iterate (up to buffer capacity).
// read whole ws message in any number of frames (up to buffer capacity).
void socket::async_read(http::flat_buffer& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        if (websocket())
        {
            buffer.consume(buffer.size());
            do_ws_read_message(std::ref(buffer), handler);
        }
        else
        {
            auto remain = floored_subtract(buffer.max_size(), buffer.size());
            if (is_zero(remain))
            {
                handler(error::buffer_overflow, {});
                return;
            }

            VARIANT_DISPATCH_METHOD(get_tcp(),
                async_read_some(buffer.prepare(remain),
                    std::bind(&socket::handle_async, shared_from_this(),
                    _1, _2, handler, "async_read_some")));
        }
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ async_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

// read tcp fixed count of bytes (waits until mutable buffer is filled).
void socket::async_read(const io::mutable_buffer& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (websocket())
    {
        // Websockets read frame not size, use async_read(flat_buffer).
        handler(error::operation_failed, {});
    }
    else
    {
        // Fixed-size semantics.
        VARIANT_DISPATCH_FUNCTION(io::async_read, get_tcp(),
            buffer, std::bind(&socket::handle_async,
                shared_from_this(), _1, _2, handler, "async_read"));
    }
}

void socket::async_read_http(http::flat_buffer& buffer, http::request& request,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (websocket())
    {
        // The body reader processes a websocket read just like an http
        // read but without the headers. The expected body type (parser)
        // must be preselected on the request object.
        body_read(buffer, request, move_copy(handler));
    }
    else
    {
        const auto parser = to_shared<http_parser>();
        parser->body_limit(maximum_);
        parser->header_limit(header_);
        do_parse(std::ref(buffer), parser, parse_mode::all, zero,
            std::bind(&socket::handle_http_read,
                shared_from_this(), _1, _2, std::ref(request), parser,
                handler));
    }
}

void socket::async_write_http(http::response&& response,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    assign_buffer(response.body());

    if (websocket())
    {
        // The body writer processes a websocket write just like an http
        // write but without the headers. The expected body type
        // (serializer) is determined by caller-assigned body.
        body_write(std::move(response), move_copy(handler));
    }
    else
    {
        const auto out = emplace_shared<serialize_state>(
            std::move(response));

        do_serialize(out, zero,
            std::bind(&socket::handle_http_write,
                shared_from_this(), _1, _2, handler));
    }
}

// Handlers.
// ----------------------------------------------------------------------------

// private
void socket::handle_async(const code& ec, size_t size,
    const count_handler& handler, const std::string& operation) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (ec == error::operation_canceled)
    {
        handler(error::channel_stopped, size);
        return;
    }


    if (ec == error::unknown)
        logx(operation, ec);

    handler(ec, size);
}

// Logging.
// ----------------------------------------------------------------------------
// private

void socket::logx(const std::string& context,
    const code& ec) const NOEXCEPT
{
    LOGX("Socket " << context << " error (" << ec.value() << ") "
        << ec.category().name() << " : " << ec.message());
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
