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
#ifndef LIBBITCOIN_NETWORK_NET_ACCEPTOR_HPP
#define LIBBITCOIN_NETWORK_NET_ACCEPTOR_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// This class is thread safe.
/// Create inbound socket connections.
/// Stop is thread safe and idempotent, may be called multiple times.
class BCT_API acceptor
  : public std::enable_shared_from_this<acceptor>, system::noncopyable,
    track<acceptor>
{
public:
    typedef std::shared_ptr<acceptor> ptr;
    typedef std::function<void(const code&, channel::ptr)> accept_handler;

    // Construct.
    // ------------------------------------------------------------------------

    /// Construct an instance.
    /// Connector strand is self-contained.
    acceptor(asio::io_context& service, const settings& settings);

    // Stop.
    // ------------------------------------------------------------------------

    /// Start the listener on the specified port (call only once).
    virtual code start(uint16_t port);

    /// Cancel work and close the acceptor (idempotent).
    /// This action is deferred to the strand, not immediately affected.
    /// Block on threadpool.join() to ensure termination of the acceptor.
    virtual void stop();

    // Methods.
    // ------------------------------------------------------------------------
    /// Subsequent accepts may only be attempted following handler invocation.
    /// May return channel_stopped, channel_timeout, success or an error code.
    /// The channel paramter is nullptr unless success is returned.

    /// Accept next connection available until stop or timeout, starts timer.
    virtual void accept(accept_handler&& handler);

protected:
    void do_stop();
    void do_accept(accept_handler handler);

private:
    void handle_accept(const code& ec, socket::ptr socket,
        const accept_handler& handler);
    void handle_timer(const code& ec, const accept_handler& handler);

    // These are thread safe.
    const settings& settings_;
    asio::io_context& service_;
    asio::strand strand_;

    // These are protected by strand.
    deadline::ptr timer_;
    asio::acceptor acceptor_;
    bool stopped_;
};

} // namespace network
} // namespace libbitcoin

#endif
