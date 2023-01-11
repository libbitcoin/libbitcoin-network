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

/// Not thread safe, virtual.
/// Create inbound socket connections.
/// Stop is thread safe and idempotent, may be called multiple times.
class BCT_API acceptor
  : public std::enable_shared_from_this<acceptor>, system::noncopyable,
    track<acceptor>
{
public:
    DEFAULT4(acceptor);

    typedef std::shared_ptr<acceptor> ptr;
    typedef std::function<void(const code&,
        const channel::ptr&)> accept_handler;

    // Construct.
    // ------------------------------------------------------------------------

    /// Construct an instance.
    acceptor(asio::strand& strand, asio::io_context& service,
        const settings& settings) NOEXCEPT;
    virtual ~acceptor() NOEXCEPT;

    // Start/stop.
    // ------------------------------------------------------------------------

    /// Start the listener on the specified port (call only once).
    virtual code start(uint16_t port) NOEXCEPT;

    /// Cancel work and close the acceptor (idempotent).
    virtual void stop() NOEXCEPT;

    // Methods.
    // ------------------------------------------------------------------------
    /// Subsequent accepts may only be attempted following handler invocation.
    /// May return operation_canceled, channel_timeout, success or error code.
    /// The channel paramter is nullptr unless success is returned.

    /// Accept next connection available until stop or timeout, starts timer.
    virtual void accept(accept_handler&& handler) NOEXCEPT;

protected:
    // These are thread safe.
    const settings& settings_;
    asio::io_context& service_;
    asio::strand& strand_;

    // These are protected by strand.
    asio::acceptor acceptor_;
    bool stopped_;

private:
    void handle_accept(const code& ec, const socket::ptr& socket,
        const accept_handler& handler) NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
