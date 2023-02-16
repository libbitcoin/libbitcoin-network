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
#include <functional>
#include <memory>
#include <unordered_set>
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

class p2p;

/// Abstract base class for maintaining a channel set, thread safe.
class BCT_API session
  : public enable_shared_from_base<session>, public reporter
{
public:
    DELETE_COPY_MOVE(session);

    /// Start the session (call from network strand).
    virtual void start(result_handler&& handler) NOEXCEPT;

    /// Stop the subscriber (call from network strand).
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

    /// Access network configuration settings.
    const network::settings& settings() const NOEXCEPT;

    /// The direction of channel initiation.
    virtual bool inbound() const NOEXCEPT = 0;

protected:
    typedef uint64_t object_key;
    typedef resubscriber<object_key> subscriber;
    typedef subscriber::handler notifier;

    /// Construct an instance (network should be started).
    session(p2p& network, uint64_t identifier) NOEXCEPT;

    /// Asserts that session is stopped.
    virtual ~session() NOEXCEPT;

    /// Invocation helpers.
    /// -----------------------------------------------------------------------

    /// Bind a method in the base or derived class (use BIND#).
    template <class Session, typename Handler, typename... Args>
    auto bind(Handler&& handler, Args&&... args) NOEXCEPT
    {
        return std::bind(std::forward<Handler>(handler),
            shared_from_base<Session>(), std::forward<Args>(args)...);
    }

    /// Channel sequence.
    /// -----------------------------------------------------------------------

    /// Perform handshake and attach protocols (call from network strand).
    virtual void start_channel(const channel::ptr& channel,
        result_handler&& started, result_handler&& stopped) NOEXCEPT;

    /// Override to change version protocol (base calls from channel strand).
    virtual void attach_handshake(const channel::ptr& channel,
        result_handler&& handler) const NOEXCEPT;

    /// Override to change channel protocols (base calls from channel strand).
    virtual void attach_protocols(const channel::ptr& channel) const NOEXCEPT;

    /// Subscriptions.
    /// -----------------------------------------------------------------------

    /// Delayed invocation, by randomized function retry_timeout_().
    virtual void defer(result_handler&& handler) NOEXCEPT;

    /// Pend/unpend a channel, for quick stop (unpend false if not pending).
    virtual void pend(const channel::ptr& channel) NOEXCEPT;
    virtual bool unpend(const channel::ptr& channel) NOEXCEPT;

    /// Subscribe to session stop notification.
    virtual void subscribe_stop(notifier&& handler) NOEXCEPT;

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
    virtual channel::ptr create_channel(const socket::ptr& socket) NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The service is stopped.
    virtual bool stopped() const NOEXCEPT;

    /// The current thread is on the network strand.
    virtual bool stranded() const NOEXCEPT;

    /// Number of entries in the address pool.
    virtual size_t address_count() const NOEXCEPT;

    /// Number of all connected channels.
    virtual size_t channel_count() const NOEXCEPT;

    /// Number of inbound connected channels.
    virtual size_t inbound_channel_count() const NOEXCEPT;

    /// Number of outbound connected channels (including manual).
    virtual size_t outbound_channel_count() const NOEXCEPT;

    /// The address protocol is disabled (ipv6 address with ipv6 disabled).
    virtual bool disabled(const config::address& address) const NOEXCEPT;

    /// The address advertises insufficient services.
    virtual bool insufficient(const config::address& address) const NOEXCEPT;

    /// The address advertises disallowed services.
    virtual bool unsupported(const config::address& address) const NOEXCEPT;

    /// The address is not whitelisted by configuration (for non-empty list).
    virtual bool whitelisted(const config::authority& authority) const NOEXCEPT;

    /// The address is blacklisted by configuration.
    virtual bool blacklisted(const config::authority& authority) const NOEXCEPT;

    /// Notify (non-seed) subscribers on channel start.
    virtual bool notify() const NOEXCEPT = 0;

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
        const result_handler& handshake) const NOEXCEPT;
    void do_handle_handshake(const code& ec, const channel::ptr& channel,
        const result_handler& start) NOEXCEPT;
    void do_attach_protocols(const channel::ptr& channel) const NOEXCEPT;
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
    const uint64_t identifier_;
    std::atomic_bool stopped_{ true };

    // These are not thread safe.
    object_key keys_{};
    subscriber stop_subscriber_;
};

} // namespace network
} // namespace libbitcoin

#endif
