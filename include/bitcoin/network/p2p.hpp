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
#ifndef LIBBITCOIN_NETWORK_P2P_HPP
#define LIBBITCOIN_NETWORK_P2P_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/hosts.hpp>
#include <bitcoin/network/message_subscriber.hpp>
#include <bitcoin/network/sessions/session_inbound.hpp>
#include <bitcoin/network/sessions/session_manual.hpp>
#include <bitcoin/network/sessions/session_outbound.hpp>
#include <bitcoin/network/sessions/session_seed.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Top level public networking interface, partly thread safe.
class BCT_API p2p
  : public enable_shared_from_base<p2p>, noncopyable
{
public:
    typedef std::shared_ptr<p2p> ptr;
    typedef message::network_address address;
    typedef std::function<void()> stop_handler;
    typedef std::function<void(bool)> truth_handler;
    typedef std::function<void(size_t)> count_handler;
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, const address&)> address_handler;
    typedef std::function<void(const code&, channel::ptr)> channel_handler;
    typedef std::function<bool(const code&, channel::ptr)> connect_handler;
    typedef subscriber<code> stop_subscriber;
    typedef resubscriber<code, channel::ptr> channel_subscriber;

    // Templates (send/receive).
    // ------------------------------------------------------------------------

    /// Send message to all connections.
    template <typename Message>
    void broadcast(const Message& message, channel_handler handle_channel,
        result_handler handle_complete)
    {
        // Safely copy the channel collection.
        const auto channels = pending_close_.collection();

        // Invoke the completion handler after send complete on all channels.
        const auto join_handler = synchronize(handle_complete, channels.size(),
            "p2p_join", synchronizer_terminate::on_count);

        // No pre-serialize, channels may have different protocol versions.
        for (const auto channel: channels)
            channel->send(message, std::bind(&p2p::handle_send, this,
                std::placeholders::_1, channel, handle_channel, join_handler));
    }

    // Constructors.
    // ------------------------------------------------------------------------

    /// Construct an instance.
    p2p(const settings& settings);

    /// Ensure all threads are coalesced.
    virtual ~p2p();

    // Start/Run sequences.
    // ------------------------------------------------------------------------

    /// Invoke startup and seeding sequence, call from constructing thread.
    virtual void start(result_handler handler);

    /// Synchronize the blockchain and then begin long running sessions,
    /// call from start result handler. Call base method to skip sync.
    virtual void run(result_handler handler);

    // Shutdown.
    // ------------------------------------------------------------------------

    /// Idempotent call to signal work stop, start may be reinvoked after.
    /// Returns the result of file save operation.
    virtual bool stop();

    /// Blocking call to coalesce all work and then terminate all threads.
    /// Call from thread that constructed this class, or don't call at all.
    /// This calls stop, and start may be reinvoked after calling this.
    virtual bool close();

    // Properties.
    // ------------------------------------------------------------------------

    /// Network configuration settings.
    virtual const settings& network_settings() const;

    /// Return the current top block identity.
    virtual config::checkpoint top_block() const;

    /// Set the current top block identity.
    virtual void set_top_block(config::checkpoint&& top);

    /// Set the current top block identity.
    virtual void set_top_block(const config::checkpoint& top);

    /// Return the current top header identity.
    virtual config::checkpoint top_header() const;

    /// Set the current top header identity.
    virtual void set_top_header(config::checkpoint&& top);

    /// Set the current top header identity.
    virtual void set_top_header(const config::checkpoint& top);

    /// Determine if the network is stopped.
    virtual bool stopped() const;

    /// Return a reference to the network threadpool.
    virtual threadpool& thread_pool();

    // Subscriptions.
    // ------------------------------------------------------------------------

    /// Subscribe to connection creation events.
    virtual void subscribe_connection(connect_handler handler);

    /// Subscribe to service stop event.
    virtual void subscribe_stop(result_handler handler);

    // Manual connections.
    // ----------------------------------------------------------------------------

    /// Maintain a connection to hostname:port.
    virtual void connect(const config::endpoint& peer);

    /// Maintain a connection to hostname:port.
    virtual void connect(const std::string& hostname, uint16_t port);

    /// Maintain a connection to hostname:port.
    /// The callback is invoked by the first connection creation only.
    virtual void connect(const std::string& hostname, uint16_t port,
        channel_handler handler);

    // Hosts collection.
    // ------------------------------------------------------------------------

    /// Get the number of addresses.
    virtual size_t address_count() const;

    /// Store an address.
    virtual code store(const address& address);

    /// Store a collection of addresses (asynchronous).
    virtual void store(const address::list& addresses, result_handler handler);

    /// Get a randomly-selected address.
    virtual code fetch_address(address& out_address) const;

    /// Get a list of stored hosts
    virtual code fetch_addresses(address::list& out_addresses) const;

    /// Remove an address.
    virtual code remove(const address& address);

    // Pending connect collection.
    // ------------------------------------------------------------------------

    /// Store a pending connection reference.
    virtual code pend(connector::ptr connector);

    /// Free a pending connection reference.
    virtual void unpend(connector::ptr connector);

    // Pending handshake collection.
    // ------------------------------------------------------------------------

    /// Store a pending connection reference.
    virtual code pend(channel::ptr channel);

    /// Test for a pending connection reference.
    virtual bool pending(uint64_t version_nonce) const;

    /// Free a pending connection reference.
    virtual void unpend(channel::ptr channel);

    // Pending close collection (open connections).
    // ------------------------------------------------------------------------

    /// Get the number of connections.
    virtual size_t connection_count() const;

    /// Store a connection.
    virtual code store(channel::ptr channel);

    /// Determine if there exists a connection to the address.
    virtual bool connected(const address& address) const;

    /// Remove a connection.
    virtual void remove(channel::ptr channel);

protected:

    /// Attach a session to the network, caller must start the session.
    template <class Session, typename... Args>
    typename Session::ptr attach(Args&&... args)
    {
        return std::make_shared<Session>(*this, std::forward<Args>(args)...);
    }

    /// Override to attach specialized sessions.
    virtual session_seed::ptr attach_seed_session();
    virtual session_manual::ptr attach_manual_session();
    virtual session_inbound::ptr attach_inbound_session();
    virtual session_outbound::ptr attach_outbound_session();

private:
    typedef bc::pending<channel> pending_channels;
    typedef bc::pending<connector> pending_connectors;

    void handle_manual_started(const code& ec, result_handler handler);
    void handle_inbound_started(const code& ec, result_handler handler);
    void handle_hosts_loaded(const code& ec, result_handler handler);
    void handle_hosts_saved(const code& ec, result_handler handler);
    void handle_send(const code& ec, channel::ptr channel,
        channel_handler handle_channel, result_handler handle_complete);

    void handle_started(const code& ec, result_handler handler);
    void handle_running(const code& ec, result_handler handler);

    // These are thread safe.
    const settings& settings_;
    std::atomic<bool> stopped_;
    bc::atomic<config::checkpoint> top_block_;
    bc::atomic<config::checkpoint> top_header_;
    bc::atomic<session_manual::ptr> manual_;
    threadpool threadpool_;
    hosts hosts_;
    pending_connectors pending_connect_;
    pending_channels pending_handshake_;
    pending_channels pending_close_;
    stop_subscriber::ptr stop_subscriber_;
    channel_subscriber::ptr channel_subscriber_;
};

} // namespace network
} // namespace libbitcoin

#endif
