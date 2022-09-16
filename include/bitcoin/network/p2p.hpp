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

/// Peer-to-Peer network class, virtual, thread safe with exceptions:
/// * attach must be called from channel strand.
/// * close must not be called concurrently or from threadpool thread.
class BCT_API p2p
  : public enable_shared_from_base<p2p>, system::noncopyable
{
public:
    typedef std::shared_ptr<p2p> ptr;
    typedef std::function<void()> stop_handler;
    typedef std::function<void(const code&)> result_handler;
    typedef std::function<void(const code&, const channel::ptr&)> channel_handler;
    typedef subscriber<const code&, const channel::ptr&> channel_subscriber;
    typedef subscriber<const code&> stop_subscriber;

    template <typename Message>
    void broadcast(const Message& message, result_handler&& handler) noexcept
    {
        boost::asio::post(strand_,
            std::bind(&p2p::do_broadcast<Message>,
                this, system::to_shared(message), std::move(handler)));
    }

    template <typename Message>
    void broadcast(Message&& message, result_handler&& handler) noexcept
    {
        boost::asio::post(strand_,
            std::bind(&p2p::do_broadcast<Message>,
                this, system::to_shared(std::move(message)),
                    std::move(handler)));
    }

    template <typename Message>
    void broadcast(const typename Message::ptr& message,
        result_handler&& handler) noexcept
    {
        boost::asio::post(strand_,
            std::bind(&p2p::do_broadcast<Message>,
                this, message, std::move(handler)));
    }

    // Constructors.
    // ------------------------------------------------------------------------

    /// Construct an instance.
    p2p(const settings& settings) noexcept;

    /// Calls close().
    virtual ~p2p() noexcept;

    // Sequences.
    // ------------------------------------------------------------------------

    /// Invoke startup and seeding sequence.
    virtual void start(result_handler&& handler) noexcept;

    /// Run inbound and outbound sessions, call from start result handler.
    virtual void run(result_handler&& handler) noexcept;

    /// Idempotent call to block on work stop, start may be reinvoked after.
    /// Must not call concurrently or from threadpool thread (see ~).
    virtual void close() noexcept;

    // Subscriptions.
    // ------------------------------------------------------------------------

    /// Subscribe to connection creation events (allowed before start).
    /// A call after close will return success but never invokes the handler.
    virtual void subscribe_connect(channel_handler&& handler,
        result_handler&& complete) noexcept;

    /// Subscribe to service stop event (allowed before start).
    /// A call after close will return success but never invokes the handler.
    virtual void subscribe_close(result_handler&& handler,
        result_handler&& complete) noexcept;

    // Manual connections.
    // ----------------------------------------------------------------------------

    /// Maintain a connection.
    virtual void connect(const config::endpoint& endpoint) noexcept;

    /// Maintain a connection, callback is invoked on each try.
    virtual void connect(const config::endpoint& endpoint,
        channel_handler&& handler) noexcept;

    // Properties.
    // ------------------------------------------------------------------------

    /// Get the number of addresses.
    virtual size_t address_count() const noexcept;

    /// Get the number of inbound channels.
    virtual size_t inbound_channel_count() const noexcept;

    /// Get the number of channels.
    virtual size_t channel_count() const noexcept;

    /// Network configuration settings.
    const settings& network_settings() const noexcept;

    /// Return a reference to the network io_context (thread safe).
    asio::io_context& service() noexcept;

    /// Return a reference to the network strand (thread safe).
    asio::strand& strand() noexcept;

protected:
    friend class session;

    /// Attach session to network, caller must start (requires strand).
    template <class Session, typename... Args>
    typename Session::ptr attach(Args&&... args) noexcept
    {
        BC_ASSERT_MSG(stranded(), "subscribe_close");

        // Sessions are attached after network start.
        const auto session = std::make_shared<Session>(*this,
            std::forward<Args>(args)...);

        // Session lifetime is ensured by the network stop subscriber.
        subscribe_close([=](const code&) noexcept
        {
            session->stop();
        });

        return session;
    }

    /// Override to attach specialized sessions.
    virtual session_seed::ptr attach_seed_session() noexcept;
    virtual session_manual::ptr attach_manual_session() noexcept;
    virtual session_inbound::ptr attach_inbound_session() noexcept;
    virtual session_outbound::ptr attach_outbound_session() noexcept;

    /// Override for test injection.
    virtual acceptor::ptr create_acceptor() noexcept;
    virtual connector::ptr create_connector() noexcept;

    /// Maintain channel state.
    virtual void pend(uint64_t nonce) noexcept;
    virtual void unpend(uint64_t nonce) noexcept;
    virtual code store(const channel::ptr& channel, bool notify,
        bool inbound) noexcept;
    virtual bool unstore(const channel::ptr& channel, bool inbound) noexcept;

    /// Maintain address pool (TODO: move to store interface).
    virtual void fetch(hosts::address_item_handler&& handler) const noexcept;
    virtual void fetches(hosts::address_items_handler&& handler) const noexcept;
    virtual void dump(const messages::address_item& address,
        result_handler&& complete) noexcept;
    virtual void save(const messages::address_item& address,
        result_handler&& complete) noexcept;
    virtual void saves(const messages::address_items& addresses,
        result_handler&& complete) noexcept;

    /// The strand is running in this thread.
    bool stranded() const noexcept;

private:
    template <typename Message>
    void do_broadcast(const typename Message::ptr& message,
        const result_handler& handler) noexcept
    {
        BC_ASSERT_MSG(stranded(), "channels_");

        for (const auto& channel: channels_)
            channel->send<Message>(message, handler);
    }

    void subscribe_close(result_handler&& handler) noexcept;
    connectors_ptr create_connectors(size_t count) noexcept;

    virtual bool closed() const noexcept;
    virtual code start_hosts() noexcept;
    virtual void stop_hosts() noexcept;

    void do_start(const result_handler& handler) noexcept;
    void do_run(const result_handler& handler) noexcept;
    void do_close() noexcept;

    void handle_start(const code& ec, const result_handler& handler) noexcept;
    void handle_run(const code& ec, const result_handler& handler) noexcept;
  
    void do_subscribe_connect(const channel_handler& handler,
        const result_handler& complete) noexcept;
    void do_subscribe_close(const result_handler& handler,
        const result_handler& complete) noexcept;

    // Distinct method names required for std::bind.
    void do_connect(const config::endpoint& endpoint) noexcept;
    void do_connect_handled(const config::endpoint& endpoint,
        const channel_handler& handler) noexcept;

    void do_fetch(const hosts::address_item_handler& handler) const noexcept;
    void do_fetches(const hosts::address_items_handler& handler) const noexcept;
    void do_save(const messages::address_item& host,
        const result_handler& complete) noexcept;
    void do_saves(const messages::address_items& hosts,
        const result_handler& complete) noexcept;

    // These are thread safe.
    const settings& settings_;
    std::atomic<size_t> channel_count_;
    std::atomic<size_t> inbound_channel_count_;

    // These are protected by strand.
    hosts hosts_;
    threadpool threadpool_;
    session_manual::ptr manual_;

    // This is thread safe.
    asio::strand strand_;

    // These are protected by strand.
    stop_subscriber::ptr stop_subscriber_;
    channel_subscriber::ptr channel_subscriber_;
    std::unordered_set<uint64_t> nonces_;
    std::unordered_set<channel::ptr> channels_;
    std::unordered_set<config::authority> authorities_;
};

} // namespace network
} // namespace libbitcoin

#endif
