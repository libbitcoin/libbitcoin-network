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
#include <bitcoin/network/net/proxy.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/socket.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Factory for variable deadline timer pointer construction (or null).
inline deadline::ptr make_throttle(const logger& log, io::strand& strand,
    uint32_t rate_limit) NOEXCEPT
{
    return to_bool(rate_limit) ?
        system::emplace_shared<deadline>(log, strand) : nullptr;
}

// Created in started state and must be stopped, as subscribers assert if not
// stopped. Subscribers may hold protocols even if service not started.
proxy::proxy(const socket::ptr& socket, uint32_t rate_limit,
    size_t maximum_backlog) NOEXCEPT
  : reporter(socket->log),
    rate_limit_(rate_limit),
    maximum_backlog_(maximum_backlog),
    socket_(socket),
    throttle_(make_throttle(socket->log, socket->strand(), rate_limit))
{
}

proxy::~proxy() NOEXCEPT
{
    BC_ASSERT_MSG(stopped(), "proxy is not stopped");
    if (!stopped()) { LOGF("~proxy is not stopped."); }
}

// Stop (socket/proxy started upon create).
// ----------------------------------------------------------------------------
// Proxy does not (must not) stop itself.

void proxy::stop(const code& ec) NOEXCEPT
{
    if (stopped())
    {
        // Hard stop ends outstanding lazy stop, otherwise nothing.
        if (!lazy(ec))
            socket_->stop();

        return;
    }

    // Open batch response closed (written) before socket stops. Batch state
    // protected by strand, so only stranded stop can close gracefully
    // (unstranded stop, e.g. shutdown, is immediate).
    if (stranded() && batched_ && parted_)
    {
        do_stop(ec);
        return;
    }

    finish_stop(ec);
}

// private
// Close open batch response, then stop (batch open rides on first part, so
// nothing to close unless parted).
void proxy::do_stop(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());
    using namespace std::placeholders;

    // Socket not yet stopped, so deferred send still holds queue. Release it
    // here so close part is not delayed by throttle.
    if (throttle_) throttle_->stop();

    // This is the final message of the channel.
    queue_close(std::bind(&proxy::handle_stop_write,
        shared_from_this(), _1, _2, ec));
}

// private
void proxy::handle_stop_write(const code&, size_t,
    const code& reason) NOEXCEPT
{
    BC_ASSERT(stranded());
    finish_stop(reason);
}

// private
// Thread safe (called directly from stop or from the strand).
void proxy::finish_stop(const code& ec) NOEXCEPT
{
    // Stop read loop, stop accepting new work, cancel pending work.
    // Graceful close issues follow-on iocontext jobs, so it is not performed
    // within socket::stop(), and subsequent socket::stop() terminates it.
    if (lazy(ec))
        socket_->lazy_stop();
    else
        socket_->stop();

    io::post(strand(),
        std::bind(&proxy::stopping, shared_from_this(), ec));
}

// A stop for a failed or canceled transport has no peer to close with.
bool proxy::lazy(const code& ec) const NOEXCEPT
{
    return !(ec == error::service_stopped
        || ec == error::channel_stopped
        || ec == error::operation_canceled
        || ec == error::operation_failed
        || ec == error::operation_timeout
        || ec == error::peer_disconnect
        || ec == error::bad_stream
        || ec == error::end_of_stream
        || ec == error::partial_message
        || ec == error::short_read
        || ec == error::tls_stream_truncated);
}

// protected
// Should not be called internally (invoked by stop()).
void proxy::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT(stranded());

    // Release any deferred send (fires pending charge with canceled).
    if (throttle_) throttle_->stop();

    // Release any http message parse in progress.
    parser_.reset();

    // Release queued writes, handlers are not invoked after stop.
    queue_.clear();
    deferred_.clear();

    // Post stop handlers to strand and clear/stop accepting subscriptions.
    // Code provides information on reason channel stopped.
    stop_subscriber_.stop(ec);
}

proxy::object_key proxy::subscribe_stop(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT(stranded());
    using namespace std::placeholders;

    const auto key = create_key();
    stop_subscriber_.subscribe(
        std::bind(&proxy::handle_stop, _1, std::move(handler)), key);

    return key;
}

// private
bool proxy::handle_stop(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    handler(ec);
    return false;
}

bool proxy::unsubscribe_stop(object_key key) NOEXCEPT
{
    BC_ASSERT(stranded());
    return stop_subscriber_.notify_one(key, error::desubscribed);
}

void proxy::subscribe_stop(result_handler&& handler,
    result_handler&& complete) NOEXCEPT
{
    if (stopped())
    {
        complete(error::channel_stopped);
        handler(error::channel_stopped);
        return;
    }

    io::dispatch(strand(),
        std::bind(&proxy::do_subscribe_stop,
            shared_from_this(), std::move(handler), std::move(complete)));
}

// private
void proxy::do_subscribe_stop(const result_handler& handler,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT(stranded());
    subscribe_stop(move_copy(handler));
    complete(error::success);
}

// private
proxy::object_key proxy::create_key() NOEXCEPT
{
    BC_ASSERT(stranded());

    if (is_zero(++keys_))
    {
        BC_ASSERT_MSG(false, "overflow");
        LOGF("Stop subscriber key overflow.");
    }

    return keys_;
}

// Signal activity.
// ----------------------------------------------------------------------------
// override reading() to update timers.

// protected
void proxy::reading() NOEXCEPT
{
}

// protected
void proxy::writing() NOEXCEPT
{
}

// private
void proxy::do_reading() NOEXCEPT
{
    io::dispatch(strand(),
        std::bind(&proxy::reading, shared_from_this()));
}

// Properties.
// ----------------------------------------------------------------------------

io::context& proxy::service() const NOEXCEPT
{
    return socket_->service();
}

io::strand& proxy::strand() NOEXCEPT
{
    return socket_->strand();
}

bool proxy::stranded() const NOEXCEPT
{
    return socket_->stranded();
}

bool proxy::stopped() const NOEXCEPT
{
    return socket_->stopped();
}

bool proxy::inbound() const NOEXCEPT
{
    return socket_->inbound();
}

bool proxy::secure() const NOEXCEPT
{
    return socket_->secure();
}

bool proxy::encrypted() const NOEXCEPT
{
    return socket_->encrypted();
}

bool proxy::websocket() const NOEXCEPT
{
    return socket_->websocket();
}

bool proxy::downgraded() const NOEXCEPT
{
    return socket_->downgraded();
}

bool proxy::detected() const NOEXCEPT
{
    return socket_->detected();
}

code proxy::accept_websocket(const http::request& request) NOEXCEPT
{
    return socket_->accept_websocket(request);
}

uint64_t proxy::received() const NOEXCEPT
{
    return received_.load();
}

uint64_t proxy::sent() const NOEXCEPT
{
    return sent_.load();
}

const config::address& proxy::address() const NOEXCEPT
{
    return socket_->address();
}

const config::endpoint& proxy::endpoint() const NOEXCEPT
{
    return socket_->endpoint();
}

const config::endpoint& proxy::binding() const NOEXCEPT
{
    return socket_->binding();
}

size_t proxy::slot() const NOEXCEPT
{
    return socket_->slot();
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
