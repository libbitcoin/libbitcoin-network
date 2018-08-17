/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/acceptor.hpp>

#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "acceptor"

using namespace std::placeholders;

static const auto reuse_address = asio::acceptor::reuse_address(true);

acceptor::acceptor(threadpool& pool, const settings& settings)
  : stopped_(true),
    pool_(pool),
    settings_(settings),
    dispatch_(pool, NAME),
    acceptor_(pool_.service()),
    CONSTRUCT_TRACK(acceptor)
{
}

acceptor::~acceptor()
{
    BITCOIN_ASSERT_MSG(stopped(), "The acceptor was not stopped.");
}

void acceptor::stop(const code&)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (!stopped())
    {
        mutex_.unlock_upgrade_and_lock();
        //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

        // This will asynchronously invoke the handler of the pending accept.
        acceptor_.cancel();

        stopped_ = true;
        //---------------------------------------------------------------------
        mutex_.unlock();
        return;
    }

    mutex_.unlock_upgrade();
    ///////////////////////////////////////////////////////////////////////////
}

// private
bool acceptor::stopped() const
{
    return stopped_;
}

// This is hardwired to listen on IPv6.
code acceptor::listen(uint16_t )
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (!stopped())
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        return error::operation_failed;
    }

    boost_code error;
    asio::endpoint endpoint(asio::tcp::v6(), settings_.inbound_port);

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    acceptor_.open(endpoint.protocol(), error);

    if (!error)
        acceptor_.set_option(reuse_address, error);

    if (!error)
        acceptor_.bind(endpoint, error);

    if (!error)
        acceptor_.listen(asio::max_connections, error);

    stopped_ = false;

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////

    return error::boost_to_error_code(error);
}

void acceptor::accept(accept_handler handler)
{
    // Critical Section
    ///////////////////////////////////////////////////////////////////////////
    mutex_.lock_upgrade();

    if (stopped())
    {
        mutex_.unlock_upgrade();
        //---------------------------------------------------------------------
        dispatch_.concurrent(handler, error::service_stopped, nullptr);
        return;
    }

    const auto socket = std::make_shared<bc::socket>(pool_);

    mutex_.unlock_upgrade_and_lock();
    //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    // async_accept will not invoke the handler within this function.
    // The bound delegate ensures handler completion before loss of scope.
    // TODO: if the accept is invoked on a thread of the acceptor, as opposed
    // to the thread of the socket, then this is unnecessary.
    acceptor_.async_accept(socket->get(),
        std::bind(&acceptor::handle_accept,
            shared_from_this(), _1, socket, handler));

    mutex_.unlock();
    ///////////////////////////////////////////////////////////////////////////
}

// private:
void acceptor::handle_accept(const boost_code& ec, socket::ptr socket,
    accept_handler handler)
{
    if (ec)
    {
        handler(error::boost_to_error_code(ec), nullptr);
        return;
    }

    // Ensure that channel is not passed as an r-value.
    const auto created = std::make_shared<channel>(pool_, socket, settings_);
    handler(error::success, created);
}

} // namespace network
} // namespace libbitcoin
