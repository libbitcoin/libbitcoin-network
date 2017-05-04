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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_HPP

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

#define PROTOCOL_ARGS(handler, args) \
    std::forward<Handler>(handler), \
    shared_from_base<Protocol>(), \
    std::forward<Args>(args)...
#define BOUND_PROTOCOL(handler, args) \
    std::bind(PROTOCOL_ARGS(handler, args))

#define PROTOCOL_ARGS_TYPE(handler, args) \
    std::forward<Handler>(handler), \
    std::shared_ptr<Protocol>(), \
    std::forward<Args>(args)...
#define BOUND_PROTOCOL_TYPE(handler, args) \
    std::bind(PROTOCOL_ARGS_TYPE(handler, args))

class p2p;

/// Virtual base class for protocol implementation, mostly thread safe.
class BCT_API protocol
  : public enable_shared_from_base<protocol>, noncopyable
{
protected:
    typedef std::function<void()> completion_handler;
    typedef std::function<void(const code&)> event_handler;
    typedef std::function<void(const code&, size_t)> count_handler;

    /// Construct an instance.
    protocol(p2p& network, channel::ptr channel, const std::string& name);

    /// Bind a method in the derived class.
    template <class Protocol, typename Handler, typename... Args>
    auto bind(Handler&& handler, Args&&... args) ->
        decltype(BOUND_PROTOCOL_TYPE(handler, args)) const
    {
        return BOUND_PROTOCOL(handler, args);
    }

    template <class Protocol, typename Handler, typename... Args>
    void dispatch_concurrent(Handler&& handler, Args&&... args)
    {
        dispatch_.concurrent(BOUND_PROTOCOL(handler, args));
    }

    /// Send a message on the channel and handle the result.
    template <class Protocol, class Message, typename Handler, typename... Args>
    void send(const Message& packet, Handler&& handler, Args&&... args)
    {
        channel_->send(packet, BOUND_PROTOCOL(handler, args));
    }

    /// Subscribe to all channel messages, blocking until subscribed.
    template <class Protocol, class Message, typename Handler, typename... Args>
    void subscribe(Handler&& handler, Args&&... args)
    {
        channel_->template subscribe<Message>(BOUND_PROTOCOL(handler, args));
    }

    /// Subscribe to the channel stop, blocking until subscribed.
    template <class Protocol, typename Handler, typename... Args>
    void subscribe_stop(Handler&& handler, Args&&... args)
    {
        channel_->subscribe_stop(BOUND_PROTOCOL(handler, args));
    }

    /// Get the address of the channel.
    virtual config::authority authority() const;

    /// Get the protocol name, for logging purposes.
    virtual const std::string& name() const;

    /// Get the channel nonce.
    virtual uint64_t nonce() const;

    /// Get the peer version message.
    virtual version_const_ptr peer_version() const;

    /// Set the peer version message.
    virtual void set_peer_version(version_const_ptr value);

    /// Get the negotiated protocol version.
    virtual uint32_t negotiated_version() const;

    /// Set the negotiated protocol version.
    virtual void set_negotiated_version(uint32_t value);

    /// Get the threadpool.
    virtual threadpool& pool();

    /// Stop the channel (and the protocol).
    virtual void stop(const code& ec);

protected:
    void handle_send(const code& ec, const std::string& command);

private:
    threadpool& pool_;
    dispatcher dispatch_;
    channel::ptr channel_;
    const std::string name_;
};

#undef PROTOCOL_ARGS
#undef BOUND_PROTOCOL
#undef PROTOCOL_ARGS_TYPE
#undef BOUND_PROTOCOL_TYPE

#define SEND1(message, method, p1) \
    send<CLASS>(message, &CLASS::method, p1)
#define SEND2(message, method, p1, p2) \
    send<CLASS>(message, &CLASS::method, p1, p2)
#define SEND3(message, method, p1, p2, p3) \
    send<CLASS>(message, &CLASS::method, p1, p2, p3)

#define SUBSCRIBE2(message, method, p1, p2) \
    subscribe<CLASS, message>(&CLASS::method, p1, p2)
#define SUBSCRIBE3(message, method, p1, p2, p3) \
    subscribe<CLASS, message>(&CLASS::method, p1, p2, p3)

#define SUBSCRIBE_STOP1(method, p1) \
    subscribe_stop<CLASS>(&CLASS::method, p1)

#define DISPATCH_CONCURRENT1(method, p1) \
    dispatch_concurrent<CLASS>(&CLASS::method, p1)

#define DISPATCH_CONCURRENT2(method, p1, p2) \
    dispatch_concurrent<CLASS>(&CLASS::method, p1, p2)

} // namespace network
} // namespace libbitcoin

#endif
