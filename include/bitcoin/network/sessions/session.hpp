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
#ifndef LIBBITCOIN_NETWORK_SESSION_HPP
#define LIBBITCOIN_NETWORK_SESSION_HPP

#include <atomic>
#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/peer/peer.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class net;
#define CLASS session

/// Abstract base class for maintaining a channel set, thread safe.
class BCT_API session
  : public enable_shared_from_base<session>, public reporter
{
public:
    typedef std::shared_ptr<session> ptr;
    typedef broadcaster::channel_id channel_id;

    DELETE_COPY_MOVE(session);

protected:
    /// Bind a method in the base or derived class (use BIND#).
    template <class Derived, typename Method, typename... Args>
    auto bind(Method&& method, Args&&... args) NOEXCEPT
    {
        return BIND_SHARED(method, args);
    }

private:
    template <typename Message>
    void do_broadcast(const typename Message::cptr& message,
        channel_id sender) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        broadcaster_.notify(message, sender);
    }

    template <typename Handler>
    void do_subscribe(const Handler& handler, channel_id subscriber) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        broadcaster_.subscribe(move_copy(handler), subscriber);
    }

    void do_unsubscribe(channel_id subscriber) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        broadcaster_.unsubscribe(subscriber);
    }

public:
    /// Broadcast.
    /// -----------------------------------------------------------------------
    /// Broadcast offers no completion handling, and subscription exists in a
    /// race with channel establishment. Broadcasts are designed for internal
    /// best-efforts propagation. Use individual channel.send calls otherwise.
    /// Sender identifies the channel to its own handler, for option to bypass.

    template <class Message, typename Handler = broadcaster::handler<Message>>
    void subscribe(Handler&& handler, channel_id id) NOEXCEPT
    {
        boost::asio::post(strand(),
            BIND(do_subscribe<Handler>, std::forward<Handler>(handler), id));
    }

    template <class Message>
    void broadcast(const typename Message::cptr& message,
        channel_id sender) NOEXCEPT
    {
        boost::asio::post(strand(),
            BIND(do_broadcast<Message>, message, sender));
    }

    virtual void unsubscribe(channel_id subscriber) NOEXCEPT
    {
        boost::asio::post(strand(),
            BIND(do_unsubscribe, subscriber));
    }

    /// Start/stop.
    /// -----------------------------------------------------------------------

    /// Start the session (call from network strand).
    virtual void start(result_handler&& handler) NOEXCEPT;

    /// Stop the session (call from network strand).
    virtual void stop() NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Access network configuration settings.
    const network::settings& settings() const NOEXCEPT;

    /// Arbitrary identifier of the session (for net subscriber).
    uint64_t identifier() const NOEXCEPT;

protected:
    typedef uint64_t object_key;
    typedef desubscriber<object_key> subscriber;
    typedef subscriber::handler notifier;

    /// Constructors.
    /// -----------------------------------------------------------------------

    /// Construct an instance (network should be started).
    session(net& network, uint64_t identifier) NOEXCEPT;

    /// Asserts that session is stopped.
    virtual ~session() NOEXCEPT;

    /// Channel sequence.
    /// -----------------------------------------------------------------------

    /// Perform handshake and attach protocols (call from network strand).
    virtual void start_channel(const channel::ptr& channel,
        result_handler&& starter, result_handler&& stopper) NOEXCEPT;

    /// Override to change version protocol (base calls from channel strand).
    virtual void attach_handshake(const channel::ptr& channel,
        result_handler&& handler) NOEXCEPT = 0;

    /// Override to change channel protocols (base calls from channel strand).
    virtual void attach_protocols(const channel::ptr& channel) NOEXCEPT = 0;

    /// Subscriptions.
    /// -----------------------------------------------------------------------

    /// Delayed invocation, by given time or randomized retry_timeout.
    virtual void defer(result_handler&& handler) NOEXCEPT;
    virtual void defer(const steady_clock::duration& delay,
        result_handler&& handler) NOEXCEPT;

    /// Pend/unpend channel, for stop notification (unpend false if not pent).
    virtual void pend(const channel::ptr& channel) NOEXCEPT;
    virtual void unpend(const channel::ptr& channel) NOEXCEPT;

    /// Subscribe to session stop notification, obtain unsubscribe key.
    virtual object_key subscribe_stop(notifier&& handler) NOEXCEPT;
    virtual bool notify(object_key key) NOEXCEPT;
    virtual object_key create_key() NOEXCEPT;

    /// Remove self from network close subscription (for session early stop).
    virtual void unsubscribe_close() NOEXCEPT;

    /// Factories.
    /// -----------------------------------------------------------------------

    /// Call to create channel acceptor.
    virtual acceptor::ptr create_acceptor() NOEXCEPT;

    /// Call to create channel connector (option for seed connection timeout).
    virtual connector::ptr create_connector(bool seed=false) NOEXCEPT;

    /// Call to create a set of channel connectors.
    virtual connectors_ptr create_connectors(size_t count) NOEXCEPT;

    /// Create a channel from the started socket.
    virtual channel::ptr create_channel(const socket::ptr& socket) NOEXCEPT = 0;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The session is stopped.
    virtual bool stopped() const NOEXCEPT;

    /// The current thread is on the network strand.
    virtual bool stranded() const NOEXCEPT;

    /// The network strand.
    asio::strand& strand() NOEXCEPT;

protected:
    // TODO: it would be preferrable for these to be made private again.
    virtual void handle_channel_starting(const code& ec,
        const channel::ptr& channel, const result_handler& started,
        const result_handler& stopped) NOEXCEPT;
    virtual void handle_handshake(const code& ec, const channel::ptr& channel,
        const result_handler& start) NOEXCEPT;
    virtual void handle_channel_started(const code& ec,
        const channel::ptr& channel, const result_handler& started) NOEXCEPT;
    virtual void handle_channel_stopped(const code& ec,
        const channel::ptr& channel, const result_handler& stopped) NOEXCEPT;

    virtual void do_attach_handshake(const channel::ptr& channel,
        const result_handler& handshake) NOEXCEPT;
    virtual void do_handle_handshake(const code& ec,
        const channel::ptr& channel, const result_handler& start) NOEXCEPT;
    virtual void do_attach_protocols(const channel::ptr& channel,
        const result_handler& started) NOEXCEPT;
    virtual void do_handle_channel_started(const code& ec,
        const channel::ptr& channel, const result_handler& started) NOEXCEPT;
    virtual void do_handle_channel_stopped(const code& ec,
        const channel::ptr& channel, const result_handler& stopped) NOEXCEPT;

private:
    void handle_timer(const code& ec, object_key key,
        const result_handler& complete) NOEXCEPT;
    bool handle_defer(const code& ec, object_key key,
        const deadline::ptr& timer) NOEXCEPT;
    bool handle_pend(const code& ec, const channel::ptr& channel) NOEXCEPT;

    // These are thread safe (mostly).
    net& network_;
    broadcaster& broadcaster_;
    const uint64_t identifier_;
    std::atomic_bool stopped_{ true };

    // This is not thread safe.
    subscriber stop_subscriber_{};
};

#undef CLASS

} // namespace network
} // namespace libbitcoin

#endif
