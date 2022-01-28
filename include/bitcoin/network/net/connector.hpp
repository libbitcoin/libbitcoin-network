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
#ifndef LIBBITCOIN_NETWORK_NET_CONNECTOR_HPP
#define LIBBITCOIN_NETWORK_NET_CONNECTOR_HPP

#include <functional>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// This class is thread safe.
/// Create outbound socket connections.
/// Stop is thread safe and idempotent, may be called multiple times.
class BCT_API connector
  : public std::enable_shared_from_this<connector>, system::noncopyable,
    track<connector>
{
public:
    typedef std::shared_ptr<connector> ptr;
    typedef std::function<void(const code& ec, channel::ptr)>
        connect_handler;

    // Construct.
    // ------------------------------------------------------------------------

    /// Construct an instance.
    /// Connector strand is self-contained.
    connector(asio::io_context& service, const settings& settings);

    // Stop.
    // ------------------------------------------------------------------------

    /// TODO: remove ec (see pend).
    /// Cancel work and close the connector (idempotent).
    /// This action is deferred to the strand, not immediately affected.
    /// Block on threadpool.join() to ensure termination of the connector.
    void stop(const code& ec);

    // Methods.
    // ------------------------------------------------------------------------
    /// A connection may only be reattempted following handler invocation.
    /// May return channel_stopped, channel_timeout, success or an error code.
    /// The channel paramter is nullptr unless success is returned.

    /// Try to connect to the endpoint, starts timer.
    virtual void connect(const config::endpoint& endpoint,
        connect_handler&& handler);

    /// Try to connect to the authority, starts timer.
    virtual void connect(const config::authority& authority,
        connect_handler&& handler);

    /// Try to connect to host:port, starts timer.
    virtual void connect(const std::string& hostname, uint16_t port,
        connect_handler&& handler);

protected:
    void do_stop();
    void do_resolve(const std::string& hostname, uint16_t port,
        connect_handler handler);

private:
    void handle_resolve(const error::boost_code& ec, const asio::resolved& it,
        socket::ptr socket, connect_handler handler);
    void handle_connect(const code& ec, socket::ptr socket,
        const connect_handler& handler);
    void handle_timer(const code& ec, const connect_handler& handler);

    // These are thread safe
    const settings& settings_;
    asio::io_context& service_;
    asio::strand strand_;

    // These are protected by strand.
    deadline::ptr timer_;
    asio::resolver resolver_;
    bool stopped_;
};

} // namespace network
} // namespace libbitcoin

#endif
