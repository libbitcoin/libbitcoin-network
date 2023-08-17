/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Boost: "The execution context provides the I/O executor that the socket will
// use, by default, to dispatch handlers for any asynchronous operations
// performed on the socket." Calls are stranded to protect the socket member.

// Construction.
// ----------------------------------------------------------------------------

// authority_.port() zero implies inbound connection.
socket::socket(const logger& log, asio::io_context& service) NOEXCEPT
  : socket(log, service, config::address{})
{
}

// authority_.port() nonzero implies outbound connection.
socket::socket(const logger& log, asio::io_context& service,
    const config::address& address) NOEXCEPT
  : address_(address),
    strand_(service.get_executor()),
    socket_(strand_),
    reporter(log),
    tracker<socket>(log)
{
}

socket::~socket() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "socket is not stopped");
    if (!stopped()) { LOGF("~socket is not stopped."); }
}

// Stop.
// ----------------------------------------------------------------------------
// The socket is not allowed to stop itself (internally).

void socket::stop() NOEXCEPT
{
    if (stopped_.load())
        return;

    // Stop flag can accelerate work stoppage, as it does not wait on strand.
    stopped_.store(true);

    // Stop is posted to strand to protect the socket.
    boost::asio::post(strand_,
        std::bind(&socket::do_stop, shared_from_this()));
}

// private
void socket::do_stop() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    error::boost_code ignore;

    // Disable future sends or receives on the socket, for graceful close.
    socket_.shutdown(asio::socket::shutdown_both, ignore);

    // Cancel asynchronous I/O operations and close socket.
    // The underlying descriptor is closed regardless of error return.
    // Any asynchronous send, receive or connect operations will be canceled
    // immediately, and will complete with the operation_aborted error.
    socket_.close(ignore);
}

// I/O.
// ----------------------------------------------------------------------------
// Boost async functions are NOT THREAD SAFE for the same socket object.
// This clarifies boost documentation: svn.boost.org/trac10/ticket/10009

void socket::accept(asio::acceptor& acceptor,
    result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(!socket_.is_open(), "accept on open socket");

    // Closure of the acceptor, not the socket, releases this handler.
    // The socket is not guarded during async_accept. This is required so the
    // acceptor may be guarded from its own strand while preserving hiding of
    // socket internals. This makes concurrent calls unsafe, however only the
    // acceptor (a socket factory) requires access to the socket at this time.
    // network::acceptor both invokes this call in the network strand and
    // initializes the asio::acceptor with the network strand. So the call to
    // acceptor.async_accept invokes its handler on that strand as well.
    try
    {
        // Dispatches on the acceptor's strand (which should be network).
        acceptor.async_accept(socket_,
            std::bind(&socket::handle_accept,
                shared_from_this(), _1, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ accept: " << e.what());
        handler(error::accept_failed);
    }
}

void socket::connect(const asio::endpoints& range,
    result_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&socket::do_connect,
            shared_from_this(), range, std::move(handler)));
}

// Read into pre-allocated buffer (bitcoin).
void socket::read(const data_slab& out, count_handler&& handler) NOEXCEPT
{
    // asio::mutable_buffer is essentially a data_slab.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_read, shared_from_this(),
            asio::mutable_buffer{ out.data(), out.size() },
                std::move(handler)));
}

void socket::write(const data_slice& in, count_handler&& handler) NOEXCEPT
{
    // asio::const_buffer is essentially a data_slice.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_write, shared_from_this(),
            asio::const_buffer{ in.data(), in.size() },
                std::move(handler)));
}

// executors (private).
// ----------------------------------------------------------------------------
// These execute on the strand to protect the member socket.

void socket::do_connect(const asio::endpoints& range,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!socket_.is_open(), "connect on open socket");

    try
    {
        // Establishes a socket connection by trying each endpoint in sequence.
        boost::asio::async_connect(socket_, range,
            std::bind(&socket::handle_connect,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_connect: " << e.what());
        handler(error::connect_failed);
    }
}

// Read into pre-allocated buffer (bitcoin).
void socket::do_read(const asio::mutable_buffer& out,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    try
    {
        // This composed operation posts all intermediate handlers to the strand.
        boost::asio::async_read(socket_, out,
            std::bind(&socket::handle_io,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_read: " << e.what());
        handler(error::operation_failed, zero);
    }
}

void socket::do_write(const asio::const_buffer& in,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    try
    {
        // This composed operation posts all intermediate handlers to the strand.
        boost::asio::async_write(socket_, in,
            std::bind(&socket::handle_io,
                shared_from_this(), _1, _2, handler));
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Exception @ do_write: " << e.what());
        handler(error::operation_failed, zero);
    }
}

// handlers (private).
// ----------------------------------------------------------------------------
// These are invoked on strand upon failure, socket cancel, or completion.

void socket::handle_accept(const error::boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    // This is running in the acceptor (not socket) execution context.
    // socket_ and authority_ are not guarded here, see comments on accept.

    if (!ec) authority_ = { socket_.remote_endpoint() };

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    // Translate other boost error code and invoke caller handler.
    const auto code = error::asio_to_error_code(ec);

    if (code == error::unknown)
    {
        LOGX("Raw accept code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code);
}

void socket::handle_connect(const error::boost_code& ec,
    const asio::endpoint& peer, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    authority_ = peer;

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    // Translate other boost error code and invoke caller handler.
    const auto code = error::asio_to_error_code(ec);

    if (code == error::unknown)
    {
        LOGX("Raw connect code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code);
}

void socket::handle_io(const error::boost_code& ec, size_t size,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    // Translate other boost error code and invoke caller handler.
    const auto code = error::asio_to_error_code(ec);

    if (code == error::unknown)
    {
        LOGX("Raw io code (" << ec.value() << ") " << ec.category().name()
            << ":" << ec.message());
    }

    handler(code, size);
}

// Properties.
// ----------------------------------------------------------------------------

const config::authority& socket::authority() const NOEXCEPT
{
    return authority_;
}

const config::address& socket::address() const NOEXCEPT
{
    return address_;
}

bool socket::inbound() const NOEXCEPT
{
    // Relies on construction and address default port of zero.
    return is_zero(address_.port());
}

bool socket::stopped() const NOEXCEPT
{
    return stopped_.load();
}

bool socket::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}

asio::strand& socket::strand() NOEXCEPT
{
    return strand_;
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
