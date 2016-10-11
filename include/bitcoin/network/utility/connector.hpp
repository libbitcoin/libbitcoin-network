/**
 * Copyright (c) 2011-2015 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef LIBBITCOIN_NETWORK_CONNECTOR_HPP
#define LIBBITCOIN_NETWORK_CONNECTOR_HPP

#include <atomic>
#include <future>
#include <functional>
#include <memory>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/collections/pending_sockets.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/utility/socket.hpp>

namespace libbitcoin {
namespace network {

/// Create outbound socket connections, thread and lock safe.
class BCT_API connector
  : public enable_shared_from_base<connector>, track<connector>
{
public:
    typedef std::shared_ptr<connector> ptr;
    typedef std::function<void(const code& ec, channel::ptr)> connect_handler;

    /// Construct an instance.
    connector(threadpool& pool, const settings& settings);

    /// This class is not copyable.
    connector(const connector&) = delete;
    void operator=(const connector&) = delete;

    /// Try to connect to the endpoint.
    virtual void connect(const config::endpoint& endpoint,
        connect_handler handler);

    /// Try to connect to the authority.
    virtual void connect(const config::authority& authority,
        connect_handler handler);

    /// Try to connect to host:port.
    virtual void connect(const std::string& hostname, uint16_t port,
        connect_handler handler);

    /// Cancel all outstanding connection attempts.
    void stop();

private:
    bool stopped() const;
    void close_socket(socket socket);
    std::shared_ptr<channel> new_channel(socket::ptr socket);

    void safe_stop();
    void safe_resolve(asio::query_ptr query, connect_handler handler);
    void safe_connect(asio::iterator iterator, socket::ptr socket,
        deadline::ptr timer, connect_handler handler, std::promise<bool>* end_flag);

    void handle_resolve(const boost_code& ec, asio::iterator iterator,
        connect_handler handler);
    void handle_timer(const code& ec, socket::ptr socket,
        connect_handler handler);
    void handle_connect(const boost_code& ec, asio::iterator iter, 
            socket::ptr socket, deadline::ptr timer, connect_handler handler,
            std::promise<bool>* end_flag);

    // These are thread safe
    std::atomic<bool> stopped_;
    threadpool& pool_;
    const settings& settings_;
    pending_sockets pending_;
    mutable dispatcher dispatch_;

    // This is protected by mutex.
    std::shared_ptr<asio::resolver> resolver_;
    mutable upgrade_mutex mutex_;
};

} // namespace network
} // namespace libbitcoin

#endif
