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
#include <bitcoin/network/connector.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "connector"

using namespace bc::config;
using namespace std::placeholders;

connector::connector(threadpool& pool, const settings& settings)
  : stopped_(false),
    pool_(pool),
    settings_(settings),
    pending_(settings_),
    dispatch_(pool, NAME),
    resolver_(pool.service()),
    CONSTRUCT_TRACK(connector)
{
}

// Stop sequence.
// ----------------------------------------------------------------------------

// public:
void connector::stop()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (!stopped_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        mutex_.unlock_upgrade_and_lock();

        // This will asynchronously invoke the handler of the pending resolve.
        resolver_.cancel();
        stopped_ = true;

        mutex_.unlock();
        //---------------------------------------------------------------------
        return;
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////

    pending_.clear();
}

bool connector::stopped() const
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    shared_lock lock(mutex_);

    return stopped_;
    ///////////////////////////////////////////////////////////////////////////
}

// Connect sequence.
// ----------------------------------------------------------------------------

// public:
void connector::connect(const endpoint& endpoint, connect_handler handler)
{
    connect(endpoint.host(), endpoint.port(), handler);
}

// public:
void connector::connect(const authority& authority, connect_handler handler)
{
    connect(authority.to_hostname(), authority.port(), handler);
}

// public:
void connector::connect(const std::string& hostname, uint16_t port,
    connect_handler handler)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (stopped_)
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        dispatch_.concurrent(handler, error::service_stopped, nullptr);
        return;
    }

    query_ = std::make_shared<asio::query>(hostname, std::to_string(port));
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    mutex_.unlock_upgrade_and_lock();

    // async_resolve will not invoke the handler within this function.
    resolver_.async_resolve(*query_,
        std::bind(&connector::handle_resolve,
            shared_from_this(), _1, _2, handler));

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

void connector::handle_resolve(const boost_code& ec, asio::iterator iterator,
    connect_handler handler)
{
    using namespace boost::asio;
    static const auto mode = synchronizer_terminate::on_error;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_shared();

    if (stopped_)
    {
        mutex_.unlock_shared();
        //---------------------------------------------------------------------
        dispatch_.concurrent(handler, error::service_stopped, nullptr);
        return;
    }

    if (ec)
    {
        mutex_.unlock_shared();
        //---------------------------------------------------------------------
        dispatch_.concurrent(handler, error::resolve_failed, nullptr);
        return;
    }

    const auto timeout = settings_.connect_timeout();
    const auto timer = std::make_shared<deadline>(pool_, timeout);
    const auto socket = std::make_shared<bc::socket>();

    // Retain a socket reference until connected, allowing connect cancelation.
    pending_.store(socket);

    // Manage the timer-connect race, returning upon first completion.
    const auto completion_handler = synchronize(handler, 1, NAME, mode);

    // This is the start of the timer sub-sequence.
    timer->start(
        std::bind(&connector::handle_timer,
            shared_from_this(), _1, socket, timer, completion_handler));

    // This is the start of the connect sub-sequence.
    // async_connect will not invoke the handler within this function.
    async_connect(socket->get(), iterator,
        std::bind(&connector::handle_connect,
            shared_from_this(), _1, _2, socket, timer, completion_handler));

    mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////
}

// Timer sequence.
// ----------------------------------------------------------------------------

// private:
void connector::handle_timer(const code& ec, socket::ptr socket, deadline::ptr,
   connect_handler handler)
{
    // Cancel any current operations on the socket.
    socket->stop();

    // This is the end of the timer sequence.
    if (ec)
        handler(ec, nullptr);
    else
        handler(error::channel_timeout, nullptr);
}

// Connect sequence.
// ----------------------------------------------------------------------------

// private:
void connector::handle_connect(const boost_code& ec, asio::iterator,
    socket::ptr socket, deadline::ptr timer, connect_handler handler)
{
    // TODO: this causes a unit test exception in debug builds.
    // Stop the timer so that this instance can be destroyed earlier.
    timer->stop();

    pending_.remove(socket);

    // This is the end of the connect sequence.
    if (ec)
        handler(error::boost_to_error_code(ec), nullptr);
    else
        handler(error::success, new_channel(socket));
}

std::shared_ptr<channel> connector::new_channel(socket::ptr socket)
{
    return std::make_shared<channel>(pool_, socket, settings_);
}

} // namespace network
} // namespace libbitcoin
