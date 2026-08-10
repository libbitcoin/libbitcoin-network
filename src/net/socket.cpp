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

#include <algorithm>
#include <utility>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/deadline.hpp>

// Boost: "The execution context provides the I/O executor that the socket will
// use, by default, to dispatch handlers for any asynchronous operations
// performed on the socket." Calls are stranded to protect the socket member.

// Boost async functions are NOT THREAD SAFE for the same socket object.
// This clarifies boost documentation: svn.boost.org/trac10/ticket/10009

namespace libbitcoin {
namespace network {

using namespace system;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Construct.
// ----------------------------------------------------------------------------

socket::socket(const logger& log, asio::context& service,
    const parameters& parameters) NOEXCEPT
  : socket(log, service, parameters, {}, {}, false, true)
{
}

socket::socket(const logger& log, asio::context& service,
    const parameters& params, const config::address& address,
    const config::endpoint& endpoint, bool proxied) NOEXCEPT
  : socket(log, service, params, address, endpoint, proxied, false)
{
}

// protected
socket::socket(const logger& log, asio::context& service,
    const parameters& params, const config::address& address,
    const config::endpoint& endpoint, bool proxied, bool inbound) NOEXCEPT
  : inbound_(inbound),
    proxied_(proxied),
    maximum_(params.maximum_request),
    strand_(service.get_executor()),
    service_(service),
    context_(params.context),
    address_(address),
    endpoint_(endpoint),
    timer_(emplace_shared<deadline>(log, strand_, params.connect_timeout)),
    socket_(std::in_place_type<asio::socket>, strand_),
    reporter(log),
    tracker<socket>(log)
{
}

socket::~socket() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "socket is not stopped");
    if (!stopped_.load()) { LOGF("~socket is not stopped."); }
}

// Properties.
// ----------------------------------------------------------------------------

asio::context& socket::service() const NOEXCEPT
{
    return service_;
}

asio::strand& socket::strand() NOEXCEPT
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

const config::address& socket::address() const NOEXCEPT
{
    return address_;
}

const config::endpoint& socket::endpoint() const NOEXCEPT
{
    return endpoint_;
}

// Context.
// ----------------------------------------------------------------------------
// protected

bool socket::secure() const NOEXCEPT
{
    // TODO: zmq::context.
    return std::holds_alternative<ref<asio::ssl::context>>(context_);
}

bool socket::encrypted() const NOEXCEPT
{
    return std::holds_alternative<ref<const privacy::context>>(context_);
}

bool socket::is_secure() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<asio::ssl::socket>(socket_) ||
        std::holds_alternative<ws::ssl::socket>(socket_);
}

bool socket::is_encrypted() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<privacy::stream>(socket_);
}

bool socket::is_base() const NOEXCEPT
{
    BC_ASSERT(stranded());
    return std::holds_alternative<asio::socket>(socket_);
}

// Variant accessors.
// ----------------------------------------------------------------------------
// protected

socket::ws_t socket::get_ws() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(websocket());

    return std::visit(overload
    {
        [](asio::socket&) NOEXCEPT -> socket::ws_t
        {
            std::terminate();
        },
        [](asio::ssl::socket&) NOEXCEPT -> socket::ws_t
        {
            std::terminate();
        },
        [](ws::socket& value) NOEXCEPT -> socket::ws_t
        {
            return std::ref(value);
        },
        [](ws::ssl::socket& value) NOEXCEPT -> socket::ws_t
        {
            return std::ref(value);
        },
        [](privacy::stream&) NOEXCEPT -> socket::ws_t
        {
            std::terminate();
        }
    }, socket_);
}

socket::tcp_t socket::get_tcp() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(!websocket());

    return std::visit(overload
    {
        [](asio::socket& value) NOEXCEPT -> socket::tcp_t
        {
            return std::ref(value);
        },
        [](asio::ssl::socket& value) NOEXCEPT -> socket::tcp_t
        {
            return std::ref(value);
        },
        [](ws::socket&) NOEXCEPT -> socket::tcp_t
        {
            std::terminate();
        },
        [](ws::ssl::socket&) NOEXCEPT -> socket::tcp_t
        {
            std::terminate();
        },
        [](privacy::stream& value) NOEXCEPT -> socket::tcp_t
        {
            return std::ref(value);
        }
    }, socket_);
}

asio::socket& socket::get_base() NOEXCEPT
{
    BC_ASSERT(stranded());

    return std::visit(overload
    {
        [](asio::socket& value) NOEXCEPT -> asio::socket&
        {
            return value;
        },
        [](asio::ssl::socket& value) NOEXCEPT -> asio::socket&
        {
            return boost::beast::get_lowest_layer(value);
        },
        [](ws::socket& value) NOEXCEPT -> asio::socket&
        {
            return boost::beast::get_lowest_layer(value);
        },
        [](ws::ssl::socket& value) NOEXCEPT -> asio::socket&
        {
            return boost::beast::get_lowest_layer(value);
        },
        [](privacy::stream& value) NOEXCEPT -> asio::socket&
        {
            return boost::beast::get_lowest_layer(value);
        }
    }, socket_);
}

asio::ssl::socket& socket::get_ssl() NOEXCEPT
{
    BC_ASSERT(stranded());
    BC_ASSERT(is_secure());

    return std::visit(overload
    {
        [](asio::socket&) NOEXCEPT -> asio::ssl::socket&
        {
            std::terminate();
        },
        [](asio::ssl::socket& value) NOEXCEPT -> asio::ssl::socket&
        {
            return value;
        },
        [](ws::socket&) NOEXCEPT -> asio::ssl::socket&
        {
            std::terminate();
        },
        [](ws::ssl::socket& value) NOEXCEPT -> asio::ssl::socket&
        {
            return value.next_layer();
        },
        [](privacy::stream&) NOEXCEPT -> asio::ssl::socket&
        {
            std::terminate();
        }
    }, socket_);
}

// Variant (ws vs. tcp)  helpers.
// ----------------------------------------------------------------------------
// protected

// write message frame (ws, finish closes the message) or unframed fixed
// size bytes (tcp). Beast applies the binary/text setting only at the start
// of a message, so it is harmless to pass it on every frame of a
// multi-frame message (see boost::beast::websocket::stream::binary).
void socket::async_write(const asio::const_buffer& buffer, bool binary,
    bool finish, const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        if (websocket())
        {
            VARIANT_DISPATCH_METHOD(get_ws(), binary(binary));
            VARIANT_DISPATCH_METHOD(get_ws(),
                async_write_some(finish, buffer, std::bind(&socket::handle_async,
                    shared_from_this(), _1, _2, handler, "async_write")));
        }
        else
        {
            VARIANT_DISPATCH_FUNCTION(boost::asio::async_write, get_tcp(),
                buffer, std::bind(&socket::handle_async,
                    shared_from_this(), _1, _2, handler, "async_write"));
        }
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ async_write: " << e.what());
        handler(error::operation_failed, {});
    }
}

// read ws frame or arbitrary tcp bytes, iterate (up to buffer capacity).
void socket::async_read_some(const asio::mutable_buffer& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        if (websocket())
        {
            VARIANT_DISPATCH_METHOD(get_ws(),
                async_read_some(buffer, std::bind(&socket::handle_async,
                    shared_from_this(), _1, _2, handler, "async_read_some")));
        }
        else
        {
            VARIANT_DISPATCH_METHOD(get_tcp(),
                async_read_some(buffer, std::bind(&socket::handle_async,
                    shared_from_this(), _1, _2, handler, "async_read_some")));
        }
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ async_read_some: " << e.what());
        handler(error::operation_failed, {});
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
            VARIANT_DISPATCH_METHOD(get_ws(),
                async_read(buffer, std::bind(&socket::handle_async,
                    shared_from_this(), _1, _2, handler, "async_read")));
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
void socket::async_read(const asio::mutable_buffer& buffer,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        if (websocket())
        {
            // Websockets read frame not size, use async_read(flat_buffer).
            handler(error::operation_failed, {});
        }
        else
        {
            // Fixed-size semantics.
            VARIANT_DISPATCH_FUNCTION(boost::asio::async_read, get_tcp(),
                buffer, std::bind(&socket::handle_async,
                    shared_from_this(), _1, _2, handler, "async_read"));
        }
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ async_read: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::async_read_http(http::flat_buffer& buffer, http::request& request,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        if (websocket())
        {
            // The body reader processes a websocket read just like a
            // beast::http::async_read but without the headers. The expected
            // body type (parser) must be preselected on the request object.
            body_read(buffer, request, move_copy(handler));
        }
        else
        {
            // Explicit parser override gives access to limits.
            auto parser = to_shared<http_parser>();
            parser->body_limit(maximum_);
            parser->header_limit(limit<uint32_t>(maximum_));

            VARIANT_DISPATCH_FUNCTION(boost::beast::http::async_read,
                get_tcp(), buffer, *parser,
                std::bind(&socket::handle_http_read,
                    shared_from_this(), _1, _2, std::ref(request), parser,
                    handler));
        }
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ async_read_http: " << e.what());
        handler(error::operation_failed, {});
    }
}

void socket::async_write_http(http::response&& response,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT(stranded());

    try
    {
        if (websocket())
        {
            // The body writer processes a websocket write just like a
            // beast::http::async_write but without the headers. The expected
            // body type (serializer) is determined by caller-assigned body.
            body_write(std::move(response), move_copy(handler));
        }
        else
        {
            const auto out = move_shared(std::move(response));
            VARIANT_DISPATCH_FUNCTION(boost::beast::http::async_write,
                get_tcp(), *out,
                std::bind(&socket::handle_http_write,
                    shared_from_this(), _1, _2, out, handler));
        }
    }
    catch (const std::exception& e)
    {
        LOGF("Exception @ async_write_http: " << e.what());
        handler(error::operation_failed, {});
    }
}

// Handlers.
// ----------------------------------------------------------------------------

// private
void socket::handle_async(const boost_code& ec, size_t size,
    const count_handler& handler, const std::string& operation) NOEXCEPT
{
    BC_ASSERT(stranded());

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    const auto code = websocket() ? error::ws_to_error_code(ec) :
        error::asio_to_error_code(ec);

    if (code == error::unknown)
        logx(operation, ec);

    handler(code, size);
}

// Logging.
// ----------------------------------------------------------------------------
// private

void socket::logx(const std::string& context,
    const boost_code& ec) const NOEXCEPT
{
    LOGX("Socket " << context << " error (" << ec.value() << ") "
        << ec.category().name() << " : " << ec.message());
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
