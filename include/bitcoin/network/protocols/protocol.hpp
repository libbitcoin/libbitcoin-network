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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_HPP

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>

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

class session;

/// Virtual base class for protocol implementation, mostly thread safe.
class BCT_API protocol
  : public enable_shared_from_base<protocol>, system::noncopyable
{
public:
    void nop() volatile;

protected:
    typedef std::function<void()> completion_handler;
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, const messages::address_items&)>
        fetches_handler;

    protocol(const session& session, channel::ptr channel);
    virtual ~protocol();

    /// Bind a method in the derived class.
    template <class Protocol, typename Handler, typename... Args>
    auto bind(Handler&& handler, Args&&... args) ->
        decltype(BOUND_PROTOCOL_TYPE(handler, args)) const
    {
        return BOUND_PROTOCOL(handler, args);
    }

    template <class Protocol, class Message, typename Handler, typename... Args>
    void send(const Message& message, Handler&& handler, Args&&... args)
    {
        channel_->send<Message>(system::to_shared(message),
            BOUND_PROTOCOL(handler, args));
    }

    template <class Protocol, class Message, typename Handler, typename... Args>
    void send(Message&& message, Handler&& handler, Args&&... args)
    {
        channel_->send<Message>(system::to_shared(std::move(message)),
            BOUND_PROTOCOL(handler, args));
    }

    template <class Protocol, class Message, typename Handler, typename... Args>
    void send(typename Message::ptr message, Handler&& handler, Args&&... args)
    {
        channel_->send<Message>(message, BOUND_PROTOCOL(handler, args));
    }

    /// Subscribe to channel messages by type.
    template <class Protocol, class Message, typename Handler, typename... Args>
    void subscribe(result_handler&& complete, Handler&& handler, Args&&... args)
    {
        channel_->template subscribe<Message>(BOUND_PROTOCOL(handler, args),
            std::forward<result_handler>(complete));
    }

    bool stranded() const;
    config::authority authority() const;
    uint64_t nonce() const;
    messages::version::ptr peer_version() const;
    void set_peer_version(messages::version::ptr value);
    uint32_t negotiated_version() const;
    void set_negotiated_version(uint32_t value);
    void stop(const code& ec);

    const settings& settings() const;
    void saves(const messages::address_items& addresses, result_handler handler={});
    void fetches(fetches_handler handler);

    virtual void handle_send(const code& ec, const std::string& command);
    virtual const std::string& name() const = 0;

private:
    channel::ptr channel_;
    const session& session_;
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
    subscribe<CLASS, message>({}, &CLASS::method, p1, p2)
#define SUBSCRIBE3(message, method, p1, p2, p3) \
    subscribe<CLASS, message>({}, &CLASS::method, p1, p2, p3)

} // namespace network
} // namespace libbitcoin

#endif
