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
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session_inbound.hpp>
#include <bitcoin/network/sessions/session_manual.hpp>
#include <bitcoin/network/sessions/session_outbound.hpp>
#include <bitcoin/network/sessions/session_seed.hpp>
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
    typedef subscriber<asio::io_context, code> stop_subscriber;
    typedef subscriber<asio::io_context, code, channel::ptr> channel_subscriber;

    // Templates.
    // ------------------------------------------------------------------------

    /// Send message to all connections, handler notified for each channel.
    template <typename Message>
    void broadcast(const Message& message, channel_handler&& handle_channel)
    {
        const auto channels = pending_close_.collection();
        for (const auto& channel: channels)
            channel->send(message, std::move(handle_channel));
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

    // Shutdown.
    // ------------------------------------------------------------------------

    /// Idempotent call to signal work stop, start may be reinvoked after.
    /// Returns the result of the hosts file save operation.
    virtual bool stop();

    /// Determine if the network is stopped.
    virtual bool stopped() const;

    // Properties.
    // ------------------------------------------------------------------------

    /// Network configuration settings.
    virtual const settings& network_settings() const;

    /// Return a reference to the network io_context.
    virtual asio::io_context& service();

    /// Return the current top block identity (for p2p handshake).
    virtual system::chain::checkpoint top_block() const;

    /// Set the current top block identity (for p2p handshake).
    virtual void set_top_block(system::chain::checkpoint&& top);

    //// TODO: move to blockchain/node.

    /////// Set the current top block identity.
    ////virtual void set_top_block(const system::chain::checkpoint& top);

    /////// Return the current top header identity.
    ////virtual system::chain::checkpoint top_header() const;

    /////// Set the current top header identity.
    ////virtual void set_top_header(system::chain::checkpoint&& top);

    /////// Set the current top header identity.
    ////virtual void set_top_header(const system::chain::checkpoint& top);

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

    // TODO: make protected.

    /// Store an address.
    virtual code store(const messages::address_item& address);

    /// Store a collection of addresses (asynchronous).
    virtual void store(const messages::address_item::list& addresses,
        result_handler handler);

    /// Get a randomly-selected address.
    virtual code fetch_address(messages::address_item& out_address) const;

    /// Get a list of stored hosts
    virtual code fetch_addresses(
        messages::address_item::list& out_addresses) const;

    /// Remove an address.
    virtual code remove(const messages::address_item& address);

    // Pending connect collection.
    // ------------------------------------------------------------------------
    // TODO: remove.

    /// Store a pending connection reference.
    virtual code pend(connector::ptr connector);

    /// Free a pending connection reference.
    virtual void unpend(connector::ptr connector);

    // Pending handshake collection.
    // ------------------------------------------------------------------------
    // TODO: make private.

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

    // TODO: make private.

    /// Store a connection.
    virtual code store(channel::ptr channel);

    /// Determine if there exists a connection to the address.
    virtual bool connected(const messages::address_item& address) const;

    /// Remove a connection.
    virtual void remove(channel::ptr channel);

protected:

    /// Attach a session to the network, caller must start the session.
    template <class Session, typename... Args>
    typename Session::ptr attach(Args&&... args)
    {
        // Sessions are attached by start (manual) and run (in/out).
        const auto session = std::make_shared<Session>(*this,
            std::forward<Args>(args)...);

        // TODO: provide a way to detach (seed session).
        // TODO: Add desubscription method to subscriber (enumerable hashmap).
        // TODO: Use channel as key and bury inclusion for protocols in base.
        // Capture the session in the p2p stop handler.
        const result_handler session_stop = [session](const code& ec)
        {
            session->stop(ec);
        };

        this->subscribe_stop(session_stop);
        return session;
    }

    /// Override to attach specialized sessions.
    virtual session_seed::ptr attach_seed_session();
    virtual session_manual::ptr attach_manual_session();
    virtual session_inbound::ptr attach_inbound_session();
    virtual session_outbound::ptr attach_outbound_session();

private:
    typedef network::pending<channel> pending_channels;
    typedef network::pending<connector> pending_connectors;

    void handle_manual_started(const code& ec, result_handler handler);
    void handle_inbound_started(const code& ec, result_handler handler);
    void handle_hosts_loaded(const code& ec, result_handler handler);

    void handle_started(const code& ec, result_handler handler);
    void handle_running(const code& ec, result_handler handler);

    // These are thread safe.
    const settings& settings_;
    std::atomic<bool> stopped_;
    atomic<system::chain::checkpoint> top_block_;
    atomic<system::chain::checkpoint> top_header_;
    atomic<session_manual::ptr> manual_;
    hosts hosts_;
    threadpool threadpool_;
    pending_connectors pending_connect_;
    pending_channels pending_handshake_;
    pending_channels pending_close_;
    stop_subscriber stop_subscriber_;
    channel_subscriber channel_subscriber_;
};

} // namespace network
} // namespace libbitcoin

#endif
