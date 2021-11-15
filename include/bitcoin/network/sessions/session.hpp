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
#include <bitcoin/network/define.hpp>
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
    typedef system::config::authority authority;
    typedef system::messages::network_address address;
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, channel::ptr)> channel_handler;
    typedef std::function<void(const code&, acceptor::ptr)> accept_handler;
    typedef std::function<void(const code&, const authority&)> host_handler;

    /// Start the session.
    virtual void start(result_handler handler);

    /// Signal stop.
    virtual void stop(const code& ec);

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
        // Protocols are attached after channel start.
        const auto protocol = std::make_shared<Protocol>(channel,
            std::forward<Args>(args)...);

        // TODO: provide a way to detach (version protocols).
        // TODO: Add desubscription method to subscriber (enumerable hashmap).
        // TODO: Use channel as key and bury inclusion for protocols in base.
        // Capture the protocol in the channel stop handler.
        channel->subscribe_stop([protocol](const code& ec)
        {
            protocol->stop(ec);
        });

        return protocol;
    }

    /// Bind a method in the derived class.
    template <class Session, typename Handler, typename... Args>
    auto bind(Handler&& handler, Args&&... args) ->
        decltype(BOUND_SESSION_TYPE(handler, args)) const
    {
        return BOUND_SESSION(handler, args);
    }

    /// Delay timing for a tight failure loop, based on configured timeout.
    inline duration cycle_delay(const code& ec)
    {
        return (ec == error::channel_timeout ||
            ec == error::service_stopped || ec == error::success) ?
                seconds(0) : settings_.connect_timeout();
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
    // Initialization order places this after privates.
    const settings& settings_;

private:
    typedef network::pending<connector> connectors;

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

protected:
    p2p& network_;
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
