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
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace std::placeholders;

// Bind throws (ok).
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Construction.
// ----------------------------------------------------------------------------
// Boost: "The execution context provides the I/O executor that the socket will
// use, by default, to dispatch handlers for any asynchronous operations
// performed on the socket." Calls are stranded to protect the socket member.

socket::socket(const logger& log, asio::io_context& service) NOEXCEPT
  : stopped_(false),
    strand_(service.get_executor()),
    socket_(strand_),
    reporter(log),
    tracker<socket>(log)
{
}

socket::~socket() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "socket is not stopped");
}

// Stop.
// ----------------------------------------------------------------------------
// The socket is not allowed to stop itself (internally).

void socket::stop() NOEXCEPT
{
    // Stop flag can accelerate work stoppage, as it does not wait on strand.
    stopped_.store(true, std::memory_order_relaxed);

    // Stop is posted to strand to protect the socket.
    boost::asio::dispatch(strand_,
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
    acceptor.async_accept(socket_,
        std::bind(&socket::handle_accept,
            shared_from_this(), _1, std::move(handler)));
}

void socket::connect(const asio::endpoints& range,
    result_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_connect,
            shared_from_this(), range, std::move(handler)));
}

// Read into pre-allocated buffer (bitcoin).
void socket::read(const data_slab& out, io_handler&& handler) NOEXCEPT
{
    // asio::mutable_buffer is essentially a data_slab.
    boost::asio::dispatch(strand_,
        std::bind(&socket::do_read, shared_from_this(),
            asio::mutable_buffer{ out.data(), out.size() },
                std::move(handler)));
}

void socket::write(const data_slice& in, io_handler&& handler) NOEXCEPT
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

    // Establishes a socket connection by trying each endpoint in a sequence.
    boost::asio::async_connect(socket_, range,
        std::bind(&socket::handle_connect,
            shared_from_this(), _1, _2, handler));
}

// Read into pre-allocated buffer (bitcoin).
void socket::do_read(const asio::mutable_buffer& out,
    const io_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // This composed operation posts all intermediate handlers to the strand.
    // However, reads must not be reinvoked before completion handler fires.
    boost::asio::async_read(socket_, out,
        std::bind(&socket::handle_read,
            shared_from_this(), _1, _2, handler));
}

// Buffer writer and bump, referenced data must remain in scope until handler.
void socket::do_write(const asio::const_buffer& in,
    const io_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Enqueue copied data reference and handler.
    queue_.emplace_back(in, handler);

    // TODO: build DoS protection around backlog rate.
    backlog_ = ceilinged_add(backlog_.load(), queue_.back().data.size());

    LOG("Queue for [" << authority() << "]: " << queue_.size()
        << " (" << backlog_.load() << " bytes)");

    // Start the loop if it wasn't already started.
    if (is_one(queue_.size()))
        write();
}

// stackoverflow.com/questions/7754695/boost-asio-async-write-how-to-not-
// interleaving-async-write-calls
void socket::write() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!queue_.empty(), "queue");

    // guarded by do_write(is_one).
    // All handlers must be invoked, so continue regardless of error state.
    auto& writer = queue_.front();

    // This composed operation posts all intermediate handlers to the strand.
    // However, writes must not be reinvoked before completion handler fires.
    boost::asio::async_write(socket_, writer.data,
        std::bind(&socket::handle_write,
            shared_from_this(), _1, _2, std::move(writer.handler)));
}

// handlers (private).
// ----------------------------------------------------------------------------
// These are invoked on strand upon failure, socket cancel, or completion.

void socket::handle_accept(const error::boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    // This is running in the acceptor (not socket) execution context.
    // socket_ and authority_ are not guarded here, see comments on accept.

    if (!ec)
        authority_ = { socket_.remote_endpoint() };

    if (error::asio_is_canceled(ec))
    {
        handler(error::operation_canceled);
        return;
    }

    // Translate other boost error code and invoke caller handler.
    handler(error::asio_to_error_code(ec));
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
    handler(error::asio_to_error_code(ec));
}

void socket::handle_read(const error::boost_code& ec, size_t size,
    const io_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    // Translate other boost error code and invoke caller handler.
    handler(error::asio_to_error_code(ec), size);
}

void socket::handle_write(const error::boost_code& ec, size_t size,
    const io_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!queue_.empty(), "queue");

    // guarded by do_write(is_one).
    backlog_ = floored_subtract(backlog_.load(), queue_.front().data.size());
    queue_.pop_front();

    LOG("Dequeue for [" << authority() << "]: " << queue_.size()
        << " (" << backlog_.load() << " bytes)");

    // All handlers must be invoked, so continue regardless of error state.
    if (!queue_.empty())
        write();

    if (error::asio_is_canceled(ec))
    {
        handler(error::channel_stopped, size);
        return;
    }

    // Translate other boost error code and invoke caller handler.
    handler(error::asio_to_error_code(ec), size);
}

// Properties.
// ----------------------------------------------------------------------------

const config::authority& socket::authority() const NOEXCEPT
{
    return authority_;
}

bool socket::stopped() const NOEXCEPT
{
    return stopped_.load(std::memory_order_relaxed);
}

bool socket::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}

size_t socket::backlog() const NOEXCEPT
{
    return backlog_.load(std::memory_order_relaxed);
}

asio::strand& socket::strand() NOEXCEPT
{
    return strand_;
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
