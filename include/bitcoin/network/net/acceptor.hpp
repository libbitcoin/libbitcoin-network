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
#ifndef LIBBITCOIN_NETWORK_NET_ACCEPTOR_HPP
#define LIBBITCOIN_NETWORK_NET_ACCEPTOR_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Not thread safe, virtual.
/// Create inbound socket connections.
/// Stop is thread safe and idempotent, may be called multiple times.
class BCT_API acceptor
  : public std::enable_shared_from_this<acceptor>, public reporter,
    protected tracker<acceptor>
{
public:
    typedef std::shared_ptr<acceptor> ptr;

    DELETE_COPY_MOVE(acceptor);

    // Construct.
    // ------------------------------------------------------------------------

    /// Construct an instance.
    acceptor(const logger& log, asio::strand& strand,
        asio::io_context& service, const settings& settings,
        std::atomic_bool& suspended) NOEXCEPT;

    /// Asserts/logs stopped.
    virtual ~acceptor() NOEXCEPT;

    // Start/stop.
    // ------------------------------------------------------------------------
    /// Starts return operation_failed if not stopped.

    /// Start the listener on all interfaces on the specified port (call once).
    virtual code start(uint16_t port) NOEXCEPT;

    /// Start the listener on the specified ip address and port (call once).
    virtual code start(const config::authority& local) NOEXCEPT;

    /// Cancel work (idempotent), handler signals completion.
    virtual void stop() NOEXCEPT;

    // Properties.
    // ------------------------------------------------------------------------

    /// The local endpoint to which this acceptor is bound (requires strand).
    virtual config::authority local() const NOEXCEPT;

    // Methods.
    // ------------------------------------------------------------------------
    /// Subsequent accepts may only be attempted following handler invocation.
    /// The socket parameter is nullptr unless success is returned.

    /// Accept next connection available until stop.
    virtual void accept(socket_handler&& handler) NOEXCEPT;

protected:
    virtual code start(const asio::endpoint& point) NOEXCEPT;

    // These are thread safe.
    const settings& settings_;
    asio::io_context& service_;
    asio::strand& strand_;
    std::atomic_bool& suspended_;

    // These are protected by strand.
    asio::acceptor acceptor_;
    bool stopped_{ true };

private:
    void handle_accept(const code& ec, const socket::ptr& socket,
        const socket_handler& handler) NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
