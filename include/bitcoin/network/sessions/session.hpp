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
#ifndef LIBBITCOIN_NETWORK_SESSION_HPP
#define LIBBITCOIN_NETWORK_SESSION_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define SESSION_ARGS(handler, args) \
    std::forward<Handler>(handler), \
    shared_from_base<Session>(), \
    std::forward<Args>(args)...
#define BOUND_SESSION(handler, args) \
    std::bind(SESSION_ARGS(handler, args))

#define SESSION_ARGS_TYPE(handler, args) \
    std::forward<Handler>(handler), \
    std::shared_ptr<Session>(), \
    std::forward<Args>(args)...
#define BOUND_SESSION_TYPE(handler, args) \
    std::bind(SESSION_ARGS_TYPE(handler, args))

class p2p;

/// Base class for maintaining the lifetime of a channel set, thread safe.
class BCT_API session
  : public enable_shared_from_base<session>, system::noncopyable
{
public:
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, channel::ptr)> channel_handler;
    typedef std::function<void(const code&, acceptor::ptr)> accept_handler;
    typedef std::function<void(const code&, const config::authority&)>
        host_handler;

    virtual void start(result_handler handler);
    virtual void stop(const code& ec);

protected:
    session(p2p& network);
    ~session();

    /// Template helpers.
    // ------------------------------------------------------------------------

    /// Attach a protocol to a channel, caller must start returned protocol.
    template <class Protocol, typename... Args>
    typename Protocol::ptr attach(channel::ptr channel, Args&&... args)
    {
        // Protocols are attached after channel start.
        const auto protocol = std::make_shared<Protocol>(channel,
            std::forward<Args>(args)...);

        // Protocol lifetime is ensured by the channel stop subscriber.
        channel->subscribe_stop([=](const code& ec){ protocol->stop(ec); });
        return protocol;
    }

    /// Bind a method in the derived class.
    template <class Session, typename Handler, typename... Args>
    auto bind(Handler&& handler, Args&&... args) ->
        decltype(BOUND_SESSION_TYPE(handler, args)) const
    {
        return BOUND_SESSION(handler, args);
    }

    /// Properties.
    // ------------------------------------------------------------------------

    virtual bool stopped() const;
    virtual bool stopped(const code& ec) const;
    virtual bool blacklisted(const config::authority& authority) const;
    virtual bool inbound() const;
    virtual bool notify() const;

    /// Delay timing for a tight failure loop, based on configured timeout.
    duration cycle_delay(const code& ec);

    /// Socket creators.
    // ------------------------------------------------------------------------

    virtual acceptor::ptr create_acceptor();
    virtual connector::ptr create_connector();

    // Registration sequence.
    //-------------------------------------------------------------------------

    /// Start a new channel with the session and bind its handlers.
    virtual void start_channel(channel::ptr channel,
        result_handler handle_started, result_handler handle_stopped);

    /// Override to attach specialized handshake protocols.
    virtual void attach_handshake(channel::ptr channel,
        result_handler handle_started);

    //-------------------------------------------------------------------------

    // This is thread safe.
    std::atomic<bool> stopped_;

    // This is not thread safe.
    p2p& network_;

private:
    void handle_handshake(const code& ec, channel::ptr channel,
        result_handler handle_started);
    void handle_start(const code& ec, channel::ptr channel,
        result_handler handle_started, result_handler handle_stopped);
    void handle_remove(const code& ec, channel::ptr channel,
        result_handler handle_stopped);
};

#undef SESSION_ARGS
#undef BOUND_SESSION
#undef SESSION_ARGS_TYPE
#undef BOUND_SESSION_TYPE

////#define CONCURRENT_DELEGATE2(method, p1, p2) \
////    concurrent_delegate<CLASS>(&CLASS::method, p1, p2)


} // namespace network
} // namespace libbitcoin

#endif
