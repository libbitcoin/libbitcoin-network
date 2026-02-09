/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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

#include <memory>
#include <utility>
#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class stoppable
{
public:
    virtual void stop(const code& ec) = 0;
    virtual ~stoppable() = default;
};

/// This class is thread safe, except for:
/// * start/started must be called on strand.
/// * setters should only be invoked during handshake.
/// Abstract base class for protocols.
/// handle_ methods are always invoked on the strand.
/// Protocol start has no failure condition.
/// A protocol can only stop its channel, not the session/network.
class BCT_API protocol
  : public virtual stoppable,
    public enable_shared_from_base<protocol>, public reporter
{
public:
    typedef std::shared_ptr<protocol> ptr;
    using channel_t = channel;
    using options_t = channel_t::options_t;

    DELETE_COPY_MOVE(protocol);

    /// The channel is stopping (called on strand by stop subscription).
    /// The stopped flag is set before this is invoked by subscriber stop.
    /// This must be called only from the channel strand (requires strand).
    virtual void stopping(const code& ec) NOEXCEPT;

private:
    template <class Message, typename Handler>
    inline bool handle_broadcast(const code& ec,
        const typename Message::cptr& message, uint64_t sender,
        const Handler& handler) NOEXCEPT
    {
        if (stopped(ec))
            return false;

        // Invoke subscriber on channel strand with given parameters.
        boost::asio::post(channel_->strand(),
            std::bind(handler, ec, message, sender));

        return true;
    }

protected:
    /// Messaging.
    /// -----------------------------------------------------------------------

    /// Bind base or derived class method (use BIND).
    template <class Derived, typename Method, typename... Args>
    inline auto bind(Method&& method, Args&&... args) NOEXCEPT
    {
        return BIND_SHARED(method, args);
    }

    /// Post base or derived class method to channel strand (use POST).
    template <class Derived, typename Method, typename... Args>
    inline auto post(Method&& method, Args&&... args) NOEXCEPT
    {
        return boost::asio::post(channel_->strand(),
            BIND_SHARED(method, args));
    }

    /// Post base or derived class method to network threadpool (use PARALLEL).
    template <class Derived, typename Method, typename... Args>
    inline auto parallel(Method&& method, Args&&... args) NOEXCEPT
    {
        return boost::asio::post(channel_->service(),
            BIND_SHARED(method, args));
    }

    /// Subscribe to messages broadcasts by type (use SUBSCRIBE_BROADCAST).
    /// Method is invoked with error::subscriber_stopped if already stopped.
    template <class Derived, class Message, typename Method, typename... Args>
    inline void subscribe_broadcast(Method&& method, Args&&... args) NOEXCEPT
    {
        BC_ASSERT(stranded());

        const auto bouncer = [self = shared_from_this(),
            handler = BIND_SHARED(method, args)]
        (const auto& ec, const typename Message::cptr& message, auto id)
        {
            return self->handle_broadcast<Message>(ec, message, id, handler);
        };

        session_->subscribe<Message>(bouncer, channel_->identifier());
    }

    /// Unsubscribe to messages broadcasts by type (use UNSUBSCRIBE_BROADCAST).
    /// Unsubscribes ALL subscribers using subscribe_broadcast for the channel.
    template <class Derived>
    inline void unsubscribe_broadcast() NOEXCEPT
    {
        BC_ASSERT(stranded());
        session_->unsubscribe(channel_->identifier());
    }

    /// Broadcast a message instance to channels (use BROADCAST).
    /// Channel identifier allows recipient-sender to self-identify.
    template <class Message>
    inline void broadcast(const typename Message::cptr& message) NOEXCEPT
    {
        BC_ASSERT(stranded());
        session_->broadcast<Message>(message, channel_->identifier());
    }

    /// Start/Stop.
    /// -----------------------------------------------------------------------

    /// Construct an instance.
    protocol(const session::ptr& session, const channel::ptr& channel) NOEXCEPT;

    /// Asserts that protocol is stopped.
    virtual ~protocol() NOEXCEPT;

    /// Set protocol started state (requires strand).
    virtual void start() NOEXCEPT;

    /// Get protocol started state (requires required).
    virtual bool started() const NOEXCEPT;

    /// Channel is stopped or code set.
    virtual bool stopped(const code& ec=error::success) const NOEXCEPT;

    /// Stop the channel (stoppable::stop).
    void stop(const code& ec) NOEXCEPT override;

    /// Pause reading from the socket, stops timers (requires strand).
    virtual void pause() NOEXCEPT;

    /// Resume reading from the socket, starts timers (requires strand).
    virtual void resume() NOEXCEPT;

    /// Monitor/unmonitor the socket for cancel/write (requires strand).
    virtual void monitor(bool value) NOEXCEPT;

    /// Seconds before channel expires, zero if expired (requires strand).
    virtual size_t remaining() const NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The current thread is on the channel strand.
    virtual bool stranded() const NOEXCEPT;

    /// The opposite endpoint of the channel.
    virtual config::endpoint opposite() const NOEXCEPT;

    /// The outbound address of the channel.
    virtual const config::address& outbound() const NOEXCEPT;

    /// The nonce of the channel.
    virtual uint64_t nonce() const NOEXCEPT;

    /// Network settings.
    virtual const network::settings& network_settings() const NOEXCEPT;

    /// Channel identifier (for broadcast identification).
    virtual uint64_t identifier() const NOEXCEPT;

    /// Handlers.
    /// -----------------------------------------------------------------------

    /// Capture send results, use for no-op send handling (logged).
    virtual void handle_send(const code& ec) NOEXCEPT;

private:
    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    const channel::ptr channel_;

    // This is thread safe.
    const session::ptr session_;

    // This is protected by strand.
    bool started_{};
};

// TODO: channel and session could be obtained using a pure virtual base method
// implemented by intermediate base protocols (e.g. client, peer). This would
// eliminate redundant storage at a minimal upcasting cost.

#define DECLARE_SEND() \
    template <class Derived, class Message, typename Method, typename... Args> \
    inline void send(Message&& message, Method&& method, Args&&... args) NOEXCEPT \
    { channel_->send(std::forward<Message>(message), BIND_SHARED(method, args)); }

#define DECLARE_SUBSCRIBE_CHANNEL() \
    template <class Derived, class Message, typename Method, typename... Args> \
    inline void subscribe_channel(Method&& method, Args&&... args) NOEXCEPT \
    { channel_->template subscribe<Message>(BIND_SHARED(method, args)); }

#define SEND(message, method, ...) \
    send<CLASS>(message, &CLASS::method, __VA_ARGS__)
#define SUBSCRIBE_CHANNEL(message, method, ...) \
    subscribe_channel<CLASS, message>(&CLASS::method, __VA_ARGS__)
#define SUBSCRIBE_BROADCAST(message, method, ...) \
    subscribe_broadcast<CLASS, message>(&CLASS::method, __VA_ARGS__)
#define BROADCAST(message, ptr) broadcast<message>(ptr)
#define UNSUBSCRIBE_BROADCAST() unsubscribe_broadcast<CLASS>()

} // namespace network
} // namespace libbitcoin

#endif
