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

    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&,
        const channel::ptr&)> channel_handler;

    /// Start the session (call from network strand).
    virtual void start(result_handler&& handler) NOEXCEPT;

    /// Stop the session timer and subscriber (call from network strand).
    virtual void stop() NOEXCEPT;

    /// Utilities.
    /// -----------------------------------------------------------------------

    /// Take an entry from address pool.
    virtual void take(hosts::address_item_handler&& handler) const NOEXCEPT;

    /// Fetch a subset of entries (count based on config) from address pool.
    virtual void fetch(hosts::address_items_handler&& handler) const NOEXCEPT;

    /// Restore an address to the address pool.
    virtual void restore(const messages::address_item& address,
        result_handler&& handler) const NOEXCEPT;

    /// Save a subset of entries (count based on config) from address pool.
    virtual void save(const messages::address::ptr& message,
        result_handler&& handler) const NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Access network configuration settings.
    const network::settings& settings() const NOEXCEPT;

protected:
    typedef subscriber<code> stop_subscriber;

    /// Construct an instance (network should be started).
    session(p2p& network) NOEXCEPT;

    /// Asserts that session is stopped.
    virtual ~session() NOEXCEPT;

    /// Macro helpers.
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

    /// Start timer with completion handler.
    virtual void start_timer(result_handler&& handler,
        const duration& timeout) NOEXCEPT;

    /// Subscribe to stop notification.
    virtual void subscribe_stop(result_handler&& handler) NOEXCEPT;

    /// Factories.
    /// -----------------------------------------------------------------------

    /// Call to create channel acceptor, owned by caller.
    virtual acceptor::ptr create_acceptor() NOEXCEPT;

    /// Call to create channel connector, owned by caller.
    virtual connector::ptr create_connector() NOEXCEPT;

    /// Call to create a set of channel connectors, owned by caller.
    virtual connectors_ptr create_connectors(size_t count) NOEXCEPT;

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

    /// The address is blacklisted by configuration.
    virtual bool blacklisted(const config::authority& authority) const NOEXCEPT;

    /// The direction of channel initiation.
    virtual bool inbound() const NOEXCEPT = 0;

    /// Notify (non-seed) subscribers on channel start.
    virtual bool notify() const NOEXCEPT = 0;

private:
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

    // These are thread safe.
    p2p& network_;
    std::atomic<bool> stopped_;

    // These are not thread safe.
    deadline::ptr timer_;
    stop_subscriber stop_subscriber_;
    std::vector<connector::ptr> connectors_{};
    std::unordered_set<channel::ptr> pending_{};
};

} // namespace network
} // namespace libbitcoin

#endif
