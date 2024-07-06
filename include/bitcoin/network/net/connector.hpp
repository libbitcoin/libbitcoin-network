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
#ifndef LIBBITCOIN_NETWORK_NET_CONNECTOR_HPP
#define LIBBITCOIN_NETWORK_NET_CONNECTOR_HPP

#include <atomic>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/deadline.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, virtual.
/// Create outbound socket connections.
/// All public/protected methods must be called from strand.
/// Stop is thread safe and idempotent, may be called multiple times.
class BCT_API connector
  : public std::enable_shared_from_this<connector>, public reporter,
    protected tracker<connector>
{
public:
    typedef std::shared_ptr<connector> ptr;

    DELETE_COPY_MOVE(connector);

    // Construct.
    // ------------------------------------------------------------------------

    /// Construct an instance.
    connector(const logger& log, asio::strand& strand,
        asio::io_context& service, const settings& settings,
        std::atomic_bool& suspended) NOEXCEPT;

    /// Asserts/logs stopped.
    virtual ~connector() NOEXCEPT;

    // Stop (no start).
    // ------------------------------------------------------------------------

    /// Cancel work (idempotent), handler signals completion.
    virtual void stop() NOEXCEPT;

    // Methods.
    // ------------------------------------------------------------------------
    /// Subsequent accepts may only be attempted following handler invocation.
    /// The socket parameter is nullptr unless success is returned.

    /// Try to connect to the address, starts timer.
    virtual void connect(const config::address& host,
        socket_handler&& handler) NOEXCEPT;

    /// Try to connect to the authority, starts timer.
    virtual void connect(const config::authority& host,
        socket_handler&& handler) NOEXCEPT;

    /// Try to connect to the endpoint, starts timer.
    virtual void connect(const config::endpoint& endpoint,
        socket_handler&& handler) NOEXCEPT;

protected:
    typedef race_speed<two, const code&, const socket::ptr&> racer;

    /// Try to connect to host:port, starts timer.
    virtual void start(const std::string& hostname, uint16_t port,
        const config::address& host, socket_handler&& handler) NOEXCEPT;

    // These are thread safe
    const settings& settings_;
    asio::io_context& service_;
    asio::strand& strand_;
    std::atomic_bool& suspended_;

    // These are protected by strand.
    asio::resolver resolver_;
    deadline::ptr timer_;
    racer racer_{};

private:
    typedef std::shared_ptr<bool> finish_ptr;

    void handle_resolve(const error::boost_code& ec,
        const asio::endpoints& range, const finish_ptr& finish,
        const socket::ptr& socket) NOEXCEPT;
    void do_handle_connect(const code& ec, const finish_ptr& finish,
        const socket::ptr& socket) NOEXCEPT;
    void handle_connect(const code& ec, const finish_ptr& finish,
        const socket::ptr& socket) NOEXCEPT;
    void handle_timer(const code& ec, const finish_ptr& finish,
        const socket::ptr& socket) NOEXCEPT;
};

typedef std_vector<connector::ptr> connectors;
typedef std::shared_ptr<connectors> connectors_ptr;

} // namespace network
} // namespace libbitcoin

#endif
