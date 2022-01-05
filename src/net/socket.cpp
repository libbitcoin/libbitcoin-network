/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace boost::asio;
using namespace std::placeholders;

// Socket creates a strand from the threadpool, posting all handlers to it.
// Socket will only self destruct when there are no more references to it.
// Once the I/O context is canceled and handlers completed, the threadpool
// will join, yet the object will not destruct if handler references remain.
// This means that all self-referential handlers must be deleted, as well as
// any cached references (such as a connection pool). Canceling the I/O service
// will release (invoke) and pent handlers, but does not clear our subscriber
// queue - this must be explicitly stopped. The asio socket will stop upon 
// destruct of this class, but the class cannot destruct until all pending
// asynchrous operation handlers have been deleted. It should be the case that
// stopping the I/O service releases all such handlers, allowing the socket to
// stop on destruct. So network stop and join of the threadpool ensures
// termination of all work with the exception of our subscribers. But
// individual channel termination requires explicit stop of the subscriber,
// socket and any other pending asynchronous calls (such as timers), since the
// strand does not support cancelation. So the socket and derived classes
// (channel) must provide explit stop methods, clearing subscribers and
// canceling all asynchronous operations.

// Construction.
// ----------------------------------------------------------------------------
// Boost: "The execution context provides the I/O executor that the socket will
// use, by default, to dispatch handlers for any asynchronous operations
// performed on the socket." Calls are stranded to protect the socket member.

socket::socket(asio::io_context& service)
  : stopped_(false),
    strand_(service),
    socket_(strand_),
    CONSTRUCT_TRACK(socket)
{
}

// Stop.
// ----------------------------------------------------------------------------
// These calls may originate from any thread.
// There is no point in calling do_stop from the destructor.


bool socket::stopped() const 
{
    return stopped_.load(std::memory_order_relaxed);
}

void socket::stop()
{
    // Stop flag can accelerate work stoppage, as it does not wait on strand,
    // but does not preempt I/O calls as they are guaranteed strand invocation.
    stopped_.store(true, std::memory_order_relaxed);

    // strand::dispatch invokes its handler directly if the strand is not busy,
    // which hopefully blocks the strand until the dispatch call completes.
    // Otherwise the handler is posted to the strand for deferred completion.
    strand_.dispatch(std::bind(&socket::do_stop, shared_from_this()));
}

// private
void socket::do_stop()
{
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
// These calls may originate from outside the strand on any thread.
// Boost async functions are NOT THREAD SAFE for the same socket object.
// This clarifies boost documentation: svn.boost.org/trac10/ticket/10009

// The socket is not guarded during async_accept. This is required so the
// acceptor may be guarded from its own strand while preserving the hiding of
// socket internals. This makes concurrent calls unsafe, however only the
// acceptor (a socket factory) requires access to the socket at this time.
void socket::accept(asio::acceptor& acceptor, result_handler&& handler)
{
    acceptor.async_accept(socket_,
        std::bind(&socket::handle_accept,
            shared_from_this(), _1, std::move(handler)));
}

void socket::connect(const asio::iterator& it, result_handler&& handler)
{
    strand_.dispatch(
        std::bind(&socket::do_connect,
            shared_from_this(), it, std::move(handler)));
}

void socket::read(const data_slab& out, io_handler&& handler)
{
    strand_.dispatch(
        std::bind(&socket::do_read, shared_from_this(),
            mutable_buffer{ out.data(), out.size() }, std::move(handler)));
}

void socket::write(const data_slice& in, io_handler&& handler)
{
    strand_.dispatch(
        std::bind(&socket::do_write, shared_from_this(),
            const_buffer{ in.data(), in.size() }, std::move(handler)));
}

// executors (private).
// ----------------------------------------------------------------------------
// These may execute outside of the strand, but are isolated from it.
// handlers are invoked on strand upon failure, socket cancel, or completion.

void socket::do_connect(const asio::iterator& it, result_handler handler)
{
    async_connect(socket_, it,
        std::bind(&socket::handle_connect,
            shared_from_this(), _1, _2, std::move(handler)));
}

void socket::do_read(const mutable_buffer& out, io_handler handler)
{
    // This composed operation posts all intermediate handlers to the strand.
    async_read(socket_, out,
        std::bind(&socket::handle_io,
            shared_from_this(), _1, _2, std::move(handler)));
}

void socket::do_write(const const_buffer& in, io_handler handler)
{
    // This composed operation posts all intermediate handlers to the strand.
    async_write(socket_, in,
        std::bind(&socket::handle_io,
            shared_from_this(), _1, _2, std::move(handler)));
}

// handlers (private).
// ----------------------------------------------------------------------------

void socket::handle_accept(const error::boost_code& ec,
    const result_handler& handler)
{
    // This is running in the acceptor or socket execution context.
    // The socket is not guarded here, see comments on the accept method.
    endpoint_.store(socket_.remote_endpoint());

    if (error::asio_is_cancelled(ec))
    {
        handler(error::channel_stopped);
        return;
    }

    // Translate other boost error code and invoke caller handler.
    handler(error::asio_to_error_code(ec));
}

void socket::handle_connect(const error::boost_code& ec,
    const asio::endpoint& peer, const result_handler& handler)
{
    BITCOIN_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    endpoint_.store(peer);

    if (error::asio_is_cancelled(ec))
    {
        handler(error::channel_stopped);
        return;
    }

    // Translate other boost error code and invoke caller handler.
    handler(error::asio_to_error_code(ec));
}

void socket::handle_io(const error::boost_code& ec, size_t size,
    const io_handler& handler)
{
    BITCOIN_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    if (error::asio_is_cancelled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    // Translate other boost error code and invoke caller handler.
    handler(error::asio_to_error_code(ec), size);
}

// Properties.
// ----------------------------------------------------------------------------
// These calls may originate from any thread.

asio::strand& socket::strand()
{
    return strand_;
}

config::authority socket::endpoint() const
{
    return endpoint_.load();
}

} // namespace network
} // namespace libbitcoin
