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
#include <bitcoin/network/utility/connector.hpp>

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/proxy.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/utility/socket.hpp>

namespace libbitcoin {
namespace network {

#define NAME "connector"
    
using namespace bc::config;
using namespace std::placeholders;

// The resolver_, pending_, and stopped_ members are protected.

connector::connector(threadpool& pool, const settings& settings)
  : stopped_(false),
    pool_(pool),
    settings_(settings),
    pending_(settings_),
    dispatch_(pool, NAME),
    resolver_(std::make_shared<asio::resolver>(pool.service())),
    CONSTRUCT_TRACK(connector)
{
}

// Stop sequence.
// ----------------------------------------------------------------------------

// public:
void connector::stop()
{
    safe_stop();
    pending_.clear();
}

void connector::safe_stop()
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (!stopped_)
    {
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
        mutex_.unlock_upgrade_and_lock();

        // This will asynchronously invoke the handler of each pending resolve.
        resolver_->cancel();
        stopped_ = true;

        mutex_.unlock();
        //---------------------------------------------------------------------
        return;
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
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
        // We preserve the asynchronous contract of the async_resolve.
        // Dispatch ensures job does not execute in the current thread.
        dispatch_.concurrent(handler, error::service_stopped, nullptr);
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return;
    }

    auto query = std::make_shared<asio::query>(hostname, std::to_string(port));

    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    mutex_.unlock_upgrade_and_lock();

    // async_resolve will not invoke the handler within this function.
    resolver_->async_resolve(*query,
        std::bind(&connector::handle_resolve,
            shared_from_this(), _1, _2, handler));

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

void connector::handle_resolve(const boost_code& ec, asio::iterator iterator,
    connect_handler handler)
{
    static const auto mode = synchronizer_terminate::on_error;

    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_shared();

    if (stopped_)
    {
        dispatch_.concurrent(handler, error::service_stopped, nullptr);
        mutex_.unlock_shared();
        //---------------------------------------------------------------------
        return;
    }

    if (ec)
    {
        dispatch_.concurrent(handler, error::resolve_failed, nullptr);
        mutex_.unlock_shared();
        //---------------------------------------------------------------------
        return;
    }

    auto do_connecting = [this, &handler](asio::iterator resolver_iterator, std::promise<bool>* end_flag)
    {
        const auto timeout = settings_.connect_timeout();
        const auto timer = std::make_shared<deadline>(pool_, timeout);
        const auto socket = std::make_shared<network::socket>(pool_);
    
        // Retain a socket reference until connected, allowing connect cancelation.
        pending_.store(socket);
    
        // Manage the socket-timer race, terminating if either fails.
        const auto handle_connect = synchronize(handler, 1, NAME, mode);
    
        // This is branch #1 of the connnect sequence.
        timer->start(
            std::bind(&connector::handle_timer,
                shared_from_this(), _1, socket, handle_connect));
    
        safe_connect(resolver_iterator, socket, timer, handle_connect, end_flag);

    };
    
    // Get all hosts under one DNS record.
    for (asio::iterator end; iterator != end; ++iterator)
    {  
        std::promise<bool> promise;
        do_connecting(iterator, &promise);
        promise.get_future().get();
    }

    mutex_.unlock_shared();
    ///////////////////////////////////////////////////////////////////////////
}

void connector::safe_connect(asio::iterator iterator, socket::ptr socket,
    deadline::ptr timer, connect_handler handler, std::promise<bool>* end_flag)
{
    // Critical Section (external)
    /////////////////////////////////////////////////////////////////////////// 
    const auto locked = socket->get_socket();

    // This is branch #2 of the connnect sequence.
    using namespace boost::asio;
    async_connect(locked->get(), iterator,
        std::bind(&connector::handle_connect,
            shared_from_this(), _1, _2, socket, timer, handler, end_flag));
    /////////////////////////////////////////////////////////////////////////// 
}

// Timer sequence.
// ----------------------------------------------------------------------------

// private:
void connector::handle_timer(const code& ec, socket::ptr socket,
   connect_handler handler)
{
    // This is the end of the timer sequence.
    if (ec)
        handler(ec, nullptr);
    else
        handler(error::channel_timeout, nullptr);

    socket->close();
}

// Connect sequence.
// ----------------------------------------------------------------------------

// private:
void connector::handle_connect(const boost_code& ec, asio::iterator iterator,
    socket::ptr socket, deadline::ptr timer, connect_handler handler, std::promise<bool>* end_flag)
{
    pending_.remove(socket);

    // This is the end of the connect sequence.
    if (ec)
        handler(error::boost_to_error_code(ec), nullptr);
    else
        handler(error::success, new_channel(socket));

    end_flag->set_value(true);

    timer->stop();
}

std::shared_ptr<channel> connector::new_channel(socket::ptr socket)
{
    return std::make_shared<channel>(pool_, socket, settings_);
}

} // namespace network
} // namespace libbitcoin
