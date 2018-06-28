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
#ifndef LIBBITCOIN_NETWORK_SESSION_HPP
#define LIBBITCOIN_NETWORK_SESSION_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/acceptor.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/connector.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/proxy.hpp>
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
  : public enable_shared_from_base<session>, noncopyable
{
public:
    typedef config::authority authority;
    typedef message::network_address address;
    typedef std::function<void(bool)> truth_handler;
    typedef std::function<void(size_t)> count_handler;
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, channel::ptr)> channel_handler;
    typedef std::function<void(const code&, acceptor::ptr)> accept_handler;
    typedef std::function<void(const code&, const authority&)> host_handler;

    /// Start the session, invokes handler once stop is registered.
    virtual void start(result_handler handler);

    /// Subscribe to receive session stop notification.
    virtual void subscribe_stop(result_handler handler);

protected:

    /// Construct an instance.
    session(p2p& network, bool notify_on_connect);

    /// Validate session stopped.
    ~session();

    /// Template helpers.
    // ------------------------------------------------------------------------

    /// Attach a protocol to a channel, caller must start the channel.
    template <class Protocol, typename... Args>
    typename Protocol::ptr attach(channel::ptr channel, Args&&... args)
    {
        return std::make_shared<Protocol>(network_, channel,
            std::forward<Args>(args)...);
    }

    /// Bind a method in the derived class.
    template <class Session, typename Handler, typename... Args>
    auto bind(Handler&& handler, Args&&... args) ->
        decltype(BOUND_SESSION_TYPE(handler, args)) const
    {
        return BOUND_SESSION(handler, args);
    }

    /// Bind a concurrent delegate to a method in the derived class.
    template <class Session, typename Handler, typename... Args>
    auto concurrent_delegate(Handler&& handler, Args&&... args) ->
        delegates::concurrent<decltype(BOUND_SESSION_TYPE(handler, args))> const
    {
        return dispatch_.concurrent_delegate(SESSION_ARGS(handler, args));
    }

    /// Invoke a method in the derived class after the specified delay.
    inline void dispatch_delayed(const asio::duration& delay,
        dispatcher::delay_handler handler) const
    {
        dispatch_.delayed(delay, handler);
    }

    /// Delay timing for a tight failure loop, based on configured timeout.
    inline asio::duration cycle_delay(const code& ec)
    {
        return (ec == error::channel_timeout || ec == error::service_stopped ||
            ec == error::success) ? asio::seconds(0) :
            settings_.connect_timeout();
    }

    /// Properties.
    // ------------------------------------------------------------------------

    virtual size_t address_count() const;
    virtual size_t connection_count() const;
    virtual code fetch_address(address& out_address) const;
    virtual bool blacklisted(const authority& authority) const;
    virtual bool stopped() const;
    virtual bool stopped(const code& ec) const;

    /// Socket creators.
    // ------------------------------------------------------------------------

    virtual acceptor::ptr create_acceptor();
    virtual connector::ptr create_connector();

    // Pending connect.
    // ------------------------------------------------------------------------

    /// Store a pending connection reference.
    virtual code pend(connector::ptr connector);

    /// Free a pending connection reference.
    virtual void unpend(connector::ptr connector);

    // Pending handshake.
    // ------------------------------------------------------------------------

    /// Store a pending connection reference.
    virtual code pend(channel::ptr channel);

    /// Free a pending connection reference.
    virtual void unpend(channel::ptr channel);

    /// Test for a pending connection reference.
    virtual bool pending(uint64_t version_nonce) const;

    // Registration sequence.
    //-------------------------------------------------------------------------

    /// Register a new channel with the session and bind its handlers.
    virtual void register_channel(channel::ptr channel,
        result_handler handle_started, result_handler handle_stopped);

    /// Start the channel, override to perform pending registration.
    virtual void start_channel(channel::ptr channel,
        result_handler handle_started);

    /// Override to attach specialized handshake protocols upon session start.
    virtual void attach_handshake_protocols(channel::ptr channel,
        result_handler handle_started);

    /// The handshake is complete, override to perform loopback check.
    virtual void handshake_complete(channel::ptr channel,
        result_handler handle_started);

    // TODO: create session_timer base class.
    // Initialization order places these after privates.
    threadpool& pool_;
    const settings& settings_;

private:
    typedef bc::pending<connector> connectors;

    void handle_stop(const code& ec);
    void handle_starting(const code& ec, channel::ptr channel,
        result_handler handle_started);
    void handle_handshake(const code& ec, channel::ptr channel,
        result_handler handle_started);
    void handle_start(const code& ec, channel::ptr channel,
        result_handler handle_started, result_handler handle_stopped);
    void handle_remove(const code& ec, channel::ptr channel,
        result_handler handle_stopped);

    // These are thread safe.
    std::atomic<bool> stopped_;
    const bool notify_on_connect_;
    p2p& network_;
    mutable dispatcher dispatch_;
};

#undef SESSION_ARGS
#undef BOUND_SESSION
#undef SESSION_ARGS_TYPE
#undef BOUND_SESSION_TYPE

#define BIND1(method, p1) \
    bind<CLASS>(&CLASS::method, p1)
#define BIND2(method, p1, p2) \
    bind<CLASS>(&CLASS::method, p1, p2)
#define BIND3(method, p1, p2, p3) \
    bind<CLASS>(&CLASS::method, p1, p2, p3)
#define BIND4(method, p1, p2, p3, p4) \
    bind<CLASS>(&CLASS::method, p1, p2, p3, p4)
#define BIND5(method, p1, p2, p3, p4, p5) \
    bind<CLASS>(&CLASS::method, p1, p2, p3, p4, p5)
#define BIND6(method, p1, p2, p3, p4, p5, p6) \
    bind<CLASS>(&CLASS::method, p1, p2, p3, p4, p5, p6)
#define BIND7(method, p1, p2, p3, p4, p5, p6, p7) \
    bind<CLASS>(&CLASS::method, p1, p2, p3, p4, p5, p6, p7)

#define CONCURRENT_DELEGATE2(method, p1, p2) \
    concurrent_delegate<CLASS>(&CLASS::method, p1, p2)


} // namespace network
} // namespace libbitcoin

#endif
