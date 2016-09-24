/**
 * Copyright (c) 2011-2016 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_SESSION_HPP
#define LIBBITCOIN_NETWORK_SESSION_HPP

#include <atomic>
#include <cstddef>
#include <functional>
#include <memory>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/collections/connections.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/proxy.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/utility/acceptor.hpp>
#include <bitcoin/network/utility/connector.hpp>

namespace libbitcoin {
namespace network {

// Base session type.

#define BASE_ARGS(handler, args) \
    std::forward<Handler>(handler), \
    shared_from_this(), \
    std::forward<Args>(args)...
#define BOUND_BASE(handler, args) \
    std::bind(BASE_ARGS(handler, args))

#define BASE_ARGS_TYPE(handler, args) \
    std::forward<Handler>(handler), \
    std::shared_ptr<session>(), \
    std::forward<Args>(args)...
#define BOUND_BASE_TYPE(handler, args) \
    std::bind(BASE_ARGS_TYPE(handler, args))

// Derived session types.

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
  : public enable_shared_from_base<session>
{
public:
    typedef config::authority authority;
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

    /// This class is not copyable.
    session(const session&) = delete;
    void operator=(const session&) = delete;

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

    /////// Dispatch a concurrent method in the derived class.
    ////template <class Session, typename Handler, typename... Args>
    ////void concurrent(Handler&& handler, Args&&... args) const
    ////{
    ////    return dispatch_.concurrent(SESSION_ARGS(handler, args));
    ////}

    /// Bind a concurrent delegate to a method in the derived class.
    template <class Session, typename Handler, typename... Args>
    auto concurrent_delegate(Handler&& handler, Args&&... args) ->
        delegates::concurrent<decltype(BOUND_SESSION_TYPE(handler, args))> const
    {
        return dispatch_.concurrent_delegate(SESSION_ARGS(handler, args));
    }

    /// Properties.
    virtual void address_count(count_handler handler) const;
    virtual void fetch_address(host_handler handler) const;
    virtual void connection_count(count_handler handler) const;
    virtual bool blacklisted(const authority& authority) const;
    virtual bool stopped() const;

    /// Socket creators.
    virtual acceptor::ptr create_acceptor();
    virtual connector::ptr create_connector();

    // Pending connections collection.
    // ------------------------------------------------------------------------

    /// Store a pending connection reference.
    virtual void pend(channel::ptr channel, result_handler handler);

    /// Free a pending connection reference.
    virtual void unpend(channel::ptr channel, result_handler handler);

    /// Test for a pending connection reference.
    virtual void pending(uint64_t version_nonce, truth_handler handler) const;

    // Registration sequence.
    //-------------------------------------------------------------------------

    /// Register a new channel with the session and bind its handlers.
    virtual void register_channel(channel::ptr channel,
        result_handler handle_started, result_handler handle_stopped);

    /// Start the channel, override to perform pending or loopback check.
    virtual void start_channel(channel::ptr channel,
        result_handler handle_started);

    /// Override to attach specialized handshake protocols upon session start.
    virtual void attach_handshake_protocols(channel::ptr channel,
        result_handler handle_started);

    // TODO: create session_timer base class.
    threadpool& pool_;
    const settings& settings_;

private:
    /// Bind a method in the base class.
    template <typename Handler, typename... Args>
    auto base_bind(Handler&& handler, Args&&... args) ->
        decltype(BOUND_BASE_TYPE(handler, args))
    {
        return BOUND_BASE(handler, args);
    }

    void do_stop_acceptor(const code& ec, acceptor::ptr connect);
    void do_stop_connector(const code& ec, connector::ptr connect);

    void do_stop_session(const code&);
    void do_remove(const code& ec, channel::ptr channel,
        result_handler handle_stopped);

    void handle_starting(const code& ec, channel::ptr channel,
        result_handler handle_started);
    void handle_handshake(const code& ec, channel::ptr channel,
        result_handler handle_started);
    void handle_start(const code& ec, channel::ptr channel,
        result_handler handle_started, result_handler handle_stopped);
    void handle_remove(const code& ec, channel::ptr channel);

    // These are thread safe.
    std::atomic<bool> stopped_;
    const bool notify_on_connect_;
    p2p& network_;
    mutable dispatcher dispatch_;
};

// Base session type.

#undef BASE_ARGS
#undef BOUND_BASE
#undef BASE_ARGS_TYPE
#undef BOUND_BASE_TYPE

// Derived session types.

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

#define CONCURRENT2(method, p1, p2) \
    concurrent_delegate<CLASS>(&CLASS::method, p1, p2)


} // namespace network
} // namespace libbitcoin

#endif
