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
#include <boost/asio.hpp>
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

/// Top level public networking interface, thread safe.
class BCT_API p2p
  : public enable_shared_from_base<p2p>, system::noncopyable
{
public:
    typedef std::shared_ptr<p2p> ptr;
    typedef std::function<void()> stop_handler;
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, channel::ptr)> channel_handler;
    typedef std::function<bool(const code&, channel::ptr)> connect_handler;
    typedef subscriber<code, channel::ptr> channel_subscriber;
    typedef subscriber<code> stop_subscriber;

    template <typename Message>
    void broadcast(const Message& message, result_handler handler)
    {
        boost::asio::post(strand_,
            std::bind(&p2p::do_broadcast<Message>,
                this, system::to_shared(message), handler));
    }

    template <typename Message>
    void broadcast(Message&& message, result_handler handler)
    {
        boost::asio::post(strand_,
            std::bind(&p2p::do_broadcast<Message>,
                this, system::to_shared(std::move(message)), handler));
    }

    template <typename Message>
    void broadcast(typename Message::ptr message, result_handler handler)
    {
        boost::asio::post(strand_,
            std::bind(&p2p::do_broadcast<Message>,
                this, message, handler));
    }

    // Constructors.
    // ------------------------------------------------------------------------
    /// Construct an instance.
    p2p(const settings& settings);

    /// Calls close().
    virtual ~p2p();

    // I/O factories.
    // ------------------------------------------------------------------------

    virtual acceptor::ptr create_acceptor();
    virtual connector::ptr create_connector();
    virtual connectors_ptr create_connectors(size_t count);

    // Sequences.
    // ------------------------------------------------------------------------

    /// Invoke startup and seeding sequence.
    virtual void start(result_handler handler);

    /// Run inbound and outbound sessions, call from start result handler.
    virtual void run(result_handler handler);

    /// Not thread safe (threadpool.clear).
    /// Idempotent call to block on work stop, start may be reinvoked after.
    virtual void close();

    // Subscriptions.
    // ------------------------------------------------------------------------

    /// Subscribe to connection creation events.
    virtual void subscribe_connect(connect_handler handler,
        result_handler complete);

    /// Subscribe to service stop event.
    virtual void subscribe_close(result_handler handler,
        result_handler complete);

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
    size_t channel_count() const;

    /// Network configuration settings.
    const settings& network_settings() const;

    /// Return a reference to the network io_context (thread safe).
    asio::io_context& service();

    /// Return a reference to the network strand (thread safe).
    /// Network strand is for sessions, but also hosts, subscribe and stop.
    asio::strand& strand();

    /// Is the strand running in this thread.
    bool stranded() const;

    // Hosts collection.
    // ------------------------------------------------------------------------

    /// Get the number of addresses (thread safe).
    size_t address_count() const;

    /// Store an address.
    void load(const messages::address_item& address,
        result_handler complete);

    /// Store a collection of addresses.
    void load(const messages::address_items& addresses,
        result_handler complete);

    /// Remove an address.
    void unload(const messages::address_item& address,
        result_handler complete);

    /// Get a randomly-selected address.
    void fetch_address(hosts::address_item_handler handler) const;

    /// Get a list of stored hosts
    void fetch_addresses(hosts::address_items_handler handler) const;

protected:
    /// Attach a session to the network, caller must start returned session.
    template <class Session, typename... Args>
    typename Session::ptr do_attach(Args&&... args)
    {
        BC_ASSERT_MSG(stranded(), "do_subscribe_close");

        // Sessions are attached after network start.
        const auto session = std::make_shared<Session>(*this,
            std::forward<Args>(args)...);

        // Session lifetime is ensured by the network stop subscriber.
        do_subscribe_close([=](const code&){ session->stop(); }, {});
        return session;
    }

    /// Override to attach specialized sessions.
    virtual session_seed::ptr attach_seed_session();
    virtual session_manual::ptr attach_manual_session();
    virtual session_inbound::ptr attach_inbound_session();
    virtual session_outbound::ptr attach_outbound_session();

private:
    template <typename Message>
    void do_broadcast(typename Message::ptr message, result_handler handler)
    {
        BC_ASSERT_MSG(strand_.running_in_this_thread(), "channels_");

        for (const auto& channel: channels_)
            channel->send<Message>(message, handler);
    }

    void handle_start(code ec, result_handler handler);
    void handle_run(code ec, result_handler handler);

    void do_start(result_handler handler);
    void do_run(result_handler handler);
    void do_close();
  
    void do_subscribe_connect(connect_handler handler, result_handler complete);
    void do_subscribe_close(result_handler handler, result_handler complete);

    // Distinct names required to bind.
    void do_connect1(const config::endpoint& endpoint);
    void do_connect2(const std::string& hostname, uint16_t port);
    void do_connect3(const std::string& hostname, uint16_t port,
        channel_handler handler);

    // hosts
    void do_load(const messages::address_item& host, result_handler complete);
    void do_loads(const messages::address_items& hosts, result_handler complete);
    void do_unload(const messages::address_item& host, result_handler complete);

    void do_fetch_address(hosts::address_item_handler handler) const;
    void do_fetch_addresses(hosts::address_items_handler handler) const;

    friend class session;
    void pend(uint64_t nonce);
    void unpend(uint64_t nonce);
    void unstore(channel::ptr channel);
    code store(channel::ptr channel, bool notify, bool inbound);

    // These are thread safe.
    const settings& settings_;
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
    std::unordered_set<config::authority> authorities_;
};

} // namespace network
} // namespace libbitcoin

#endif
