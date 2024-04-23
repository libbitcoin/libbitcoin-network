/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p;
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
        const auto bouncer = [self = shared_from_this(),
            handler = std::move(handler), id]()
        {
            self->do_subscribe<Handler>(handler, id);
        };
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

    /// Utilities.
    /// -----------------------------------------------------------------------

    /// Take an entry from address pool.
    virtual void take(address_item_handler&& handler) const NOEXCEPT;

    /// Fetch a subset of entries (count based on config) from address pool.
    virtual void fetch(address_handler&& handler) const NOEXCEPT;

    /// Restore an address to the address pool.
    virtual void restore(const address_item_cptr& address,
        result_handler&& handler) const NOEXCEPT;

    /// Save a subset of entries (count based on config) from address pool.
    virtual void save(const address_cptr& message,
        count_handler&& handler) const NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Arbitrary identifier of the session (for p2p subscriber).
    uint64_t identifier() const NOEXCEPT;

    /// Access network configuration settings.
    const network::settings& settings() const NOEXCEPT;

    /// Number of entries in the address pool.
    virtual size_t address_count() const NOEXCEPT;

protected:
    typedef uint64_t object_key;
    typedef desubscriber<object_key> subscriber;
    typedef subscriber::handler notifier;

    /// Constructors.
    /// -----------------------------------------------------------------------

    /// Construct an instance (network should be started).
    session(p2p& network, uint64_t identifier) NOEXCEPT;

    /// Asserts that session is stopped.
    virtual ~session() NOEXCEPT;

    /// Channel sequence.
    /// -----------------------------------------------------------------------

    /// Perform handshake and attach protocols (call from network strand).
    virtual void start_channel(const channel::ptr& channel,
        result_handler&& starter, result_handler&& stopper) NOEXCEPT;

    /// Override to change version protocol (base calls from channel strand).
    virtual void attach_handshake(const channel::ptr& channel,
        result_handler&& handler) NOEXCEPT;

    /// Override to change channel protocols (base calls from channel strand).
    virtual void attach_protocols(const channel::ptr& channel) NOEXCEPT;

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

    /// Remove self from network close subscription (for session early stop).
    virtual void unsubscribe_close() NOEXCEPT;

    /// Factories.
    /// -----------------------------------------------------------------------

    /// Call to create channel acceptor, owned by caller.
    virtual acceptor::ptr create_acceptor() NOEXCEPT;

    /// Call to create channel connector, owned by caller.
    virtual connector::ptr create_connector() NOEXCEPT;

    /// Call to create a set of channel connectors, owned by caller.
    virtual connectors_ptr create_connectors(size_t count) NOEXCEPT;

    /// Create a channel from the started socket.
    virtual channel::ptr create_channel(const socket::ptr& socket,
        bool quiet) NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The session is stopped.
    virtual bool stopped() const NOEXCEPT;

    /// The current thread is on the network strand.
    virtual bool stranded() const NOEXCEPT;

    /// Number of all connected channels.
    virtual size_t channel_count() const NOEXCEPT;

    /// Number of inbound connected channels.
    virtual size_t inbound_channel_count() const NOEXCEPT;

    /// Number of outbound connected channels (including manual).
    virtual size_t outbound_channel_count() const NOEXCEPT;

    /// The network strand.
    asio::strand& strand() NOEXCEPT;

private:
    object_key create_key() NOEXCEPT;

    void handle_channel_start(const code& ec, const channel::ptr& channel,
        const result_handler& started, const result_handler& stopped) NOEXCEPT;

    void handle_handshake(const code& ec, const channel::ptr& channel,
        const result_handler& start) NOEXCEPT;
    void handle_channel_started(const code& ec, const channel::ptr& channel,
        const result_handler& started) NOEXCEPT;
    void handle_channel_stopped(const code& ec,const channel::ptr& channel,
        const result_handler& stopped) NOEXCEPT;

    void do_attach_handshake(const channel::ptr& channel,
        const result_handler& handshake) NOEXCEPT;
    void do_handle_handshake(const code& ec, const channel::ptr& channel,
        const result_handler& start) NOEXCEPT;
    void do_attach_protocols(const channel::ptr& channel,
        const result_handler& started) NOEXCEPT;
    void do_handle_channel_started(const code& ec, const channel::ptr& channel,
        const result_handler& started) NOEXCEPT;
    void do_handle_channel_stopped(const code& ec, const channel::ptr& channel,
        const result_handler& stopped) NOEXCEPT;

    void handle_timer(const code& ec, object_key key,
        const result_handler& complete) NOEXCEPT;
    bool handle_defer(const code& ec, object_key key,
        const deadline::ptr& timer) NOEXCEPT;
    bool handle_pend(const code& ec, const channel::ptr& channel) NOEXCEPT;

    // These are thread safe (mostly).
    p2p& network_;
    broadcaster& broadcaster_;
    const uint64_t identifier_;
    std::atomic_bool stopped_{ true };

    // These are not thread safe.
    object_key keys_{};
    subscriber stop_subscriber_;
};

#undef CLASS

} // namespace network
} // namespace libbitcoin

#endif
