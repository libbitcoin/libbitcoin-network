/**
 * Copyright (c) 2011-2018 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_ACCEPTOR_HPP
#define LIBBITCOIN_NETWORK_ACCEPTOR_HPP

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Create inbound socket connections.
/// This class is thread safe against stop.
/// This class is not safe for concurrent listening attempts.
class BCT_API acceptor
  : public system::enable_shared_from_base<acceptor>, system::noncopyable,
    track<acceptor>
{
public:
    typedef std::shared_ptr<acceptor> ptr;
    typedef std::function<void(const system::code&, channel::ptr)> accept_handler;

    /// Construct an instance.
    acceptor(system::threadpool& pool, const settings& settings);

    /// Validate acceptor stopped.
    ~acceptor();

    /// Start the listener on the specified port.
    virtual system::code listen(uint16_t port);

    /// Accept the next connection available, until canceled.
    virtual void accept(accept_handler handler);

    /// Cancel outstanding accept attempt.
    virtual void stop(const system::code& ec);

private:
    virtual bool stopped() const;

    void handle_accept(const system::boost_code& ec,
        system::socket::ptr socket, accept_handler handler);

    // These are thread safe.
    std::atomic<bool> stopped_;
    system::threadpool& pool_;
    const settings& settings_;
    mutable system::dispatcher dispatch_;

    // These are protected by mutex.
    system::asio::acceptor acceptor_;
    mutable system::shared_mutex mutex_;
};

} // namespace network
} // namespace libbitcoin

#endif
