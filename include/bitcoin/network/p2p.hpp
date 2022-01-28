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
#ifndef LIBBITCOIN_NETWORK_P2P_HPP
#define LIBBITCOIN_NETWORK_P2P_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Top level public networking interface, partly thread safe.
class BCT_API p2p
  : public enable_shared_from_base<p2p>, system::noncopyable
{
public:
    typedef std::shared_ptr<p2p> ptr;
    typedef std::function<void()> stop_handler;
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, channel::ptr)> channel_handler;
    typedef std::function<bool(const code&, channel::ptr)> connect_handler;
    typedef subscriber<code> stop_subscriber;
    typedef subscriber<code, channel::ptr> channel_subscriber;

    template <typename Message>
    void broadcast(Message&& message, result_handler handler)
    {
        post(strand_, std::bind(&p2p::do_broadcast<Message>, this,
            system::to_shared(std::move(message)), handler));
    }

    template <typename Message>
    void broadcast(typename Message::ptr message, result_handler handler)
    {
        post(strand_, std::bind(&p2p::do_broadcast<Message>, this, message, std::ref(handler)));
    }

    /// Broadcast a message to all peers.
    template <typename Message>
    void do_broadcast(typename Message::ptr message, result_handler handler)
    {
        BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

        for (const auto& channel: channels_)
            channel->send<Message>(message, handler);
    }

    // Constructors.
    // ------------------------------------------------------------------------
    /// Construct an instance.
    p2p(const settings& settings);

    /// Calls stop().
    virtual ~p2p();

    // Sequences.
    // ------------------------------------------------------------------------

    /// Invoke startup and seeding sequence.
    virtual void start(result_handler handler);

    /// Run inbound and outbound sessions, call from start result handler.
    virtual void run(result_handler handler);

    /// Idempotent call to signal work stop, start may be reinvoked after.
    /// Returns the result of the hosts file save operation.
    virtual void stop();

    /// Determine if the network is stopped.
    virtual bool stopped() const;

    // Subscriptions.
    // ------------------------------------------------------------------------

    /// Subscribe to connection creation events.
    virtual void subscribe_connect(connect_handler handler);

    /// Subscribe to service stop event.
    virtual void subscribe_close(result_handler handler);

    // Manual connections.
    // ----------------------------------------------------------------------------

    /// Maintain a connection to hostname:port.
    virtual void connect(const config::endpoint& endpoint);

    /// Maintain a connection to hostname:port.
    virtual void connect(const std::string& hostname, uint16_t port);

    /// Maintain a connection to hostname:port.
    /// The callback is invoked by the first connection creation only.
    virtual void connect(const std::string& hostname, uint16_t port,
        channel_handler handler);

    // Properties.
    // ------------------------------------------------------------------------

    /// Network configuration settings.
    virtual const settings& network_settings() const;

    /// Return a reference to the network io_context (thread safe).
    virtual asio::io_context& service();

    /// Return a reference to the network strand (thread safe).
    /// Network strand is for sessions, but also hosts, subscribe and stop.
    virtual asio::strand& strand();

    // Hosts collection.
    // ------------------------------------------------------------------------

    /// Get the number of addresses (thread safe).
    virtual size_t address_count() const;

    /// Store an address.
    virtual void load(const messages::address_item& address);

    /// Store a collection of addresses.
    virtual void load(const messages::address_items& addresses);

    /// Remove an address.
    virtual void unload(const messages::address_item& address);

    /// Get a randomly-selected address.
    virtual void fetch_address(hosts::peer_handler handler) const;

    /// Get a list of stored hosts
    virtual void fetch_addresses(hosts::peers_handler handler) const;

    // Connection management.
    // ------------------------------------------------------------------------

    ////virtual code pend(connector::ptr connector);
    ////virtual void unpend(connector::ptr connector);

    virtual size_t channel_count() const;
    virtual void pend(uint64_t nonce);
    virtual void unpend(uint64_t nonce);    
    virtual void store(channel::ptr channel, bool notify, bool inbound,
        result_handler handler);
    virtual void unstore(channel::ptr channel);

protected:
    /// Attach a session to the network, caller must start returned session.
    template <class Session, typename... Args>
    typename Session::ptr attach(Args&&... args)
    {
        // Sessions are attached after network start.
        const auto session = std::make_shared<Session>(*this,
            std::forward<Args>(args)...);

        // Session lifetime is ensured by the network stop subscriber.
        subscribe_close([=](const code& ec){ session->stop(ec); });
        return session;
    }

    /// Override to attach specialized sessions.
    virtual session_seed::ptr attach_seed_session();
    virtual session_manual::ptr attach_manual_session();
    virtual session_inbound::ptr attach_inbound_session();
    virtual session_outbound::ptr attach_outbound_session();

private:
    void handle_manual_started(const code& ec, result_handler handler);
    void handle_inbound_started(const code& ec, result_handler handler);
    void handle_hosts_loaded(const code& ec, result_handler handler);

    void handle_started(const code& ec, result_handler handler);
    void handle_running(const code& ec, result_handler handler);
    
    void do_stop();
    void do_subscribe_connect(connect_handler handler);
    void do_subscribe_close(result_handler handler);

    void do_load(const messages::address_item& host);
    void do_loads(const messages::address_items& hosts);
    void do_unload(const messages::address_item& host);

    void do_fetch_address(hosts::peer_handler handler) const;
    void do_fetch_addresses(hosts::peers_handler handler) const;

    void do_pend(uint64_t nonce);
    void do_unpend(uint64_t nonce);

    void do_store(channel::ptr channel, bool notify, bool inbound,
        result_handler handler);
    void do_unstore(channel::ptr channel);

    // These are thread safe.
    const settings& settings_;
    std::atomic<bool> stopped_;
    std::atomic<size_t> channel_count_;

    // These are not thread safe.
    hosts hosts_;
    session_manual::ptr manual_;
    threadpool threadpool_;

    // This is thread safe.
    asio::strand strand_;

    // These are not thread safe.
    stop_subscriber::ptr stop_subscriber_;
    channel_subscriber::ptr channel_subscriber_;
    std::unordered_set<uint64_t> nonces_;
    std::unordered_set<channel::ptr> channels_;
    std::unordered_set<config::authority> addresses_;
};

} // namespace network
} // namespace libbitcoin

#endif
