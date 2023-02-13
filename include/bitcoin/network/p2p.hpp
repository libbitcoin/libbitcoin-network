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
#include <functional>
#include <memory>
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

/// Peer-to-Peer network class, virtual, thread safe with exceptions:
/// * attach must be called from channel strand.
/// * close must not be called concurrently or from any threadpool thread.
class BCT_API p2p
  : public reporter
{
public:
    typedef std::shared_ptr<p2p> ptr;
    typedef resubscriber<size_t> stop_subscriber;
    typedef stop_subscriber::handler stop_handler;
    typedef stop_subscriber::completer stop_completer;
    typedef stop_subscriber::key stop_key;

    template <typename Message>
    void broadcast(const Message& message, result_handler&& handler,
        uint64_t id=zero) NOEXCEPT
    {
        boost::asio::post(strand_,
            std::bind(&p2p::do_broadcast<Message>,
                this, system::to_shared(message), id, std::move(handler)));
    }

    template <typename Message>
    void broadcast(Message&& message, result_handler&& handler,
        uint64_t id=zero) NOEXCEPT
    {
        boost::asio::post(strand_,
            std::bind(&p2p::do_broadcast<Message>,
                this, system::to_shared(std::move(message)), id,
                    std::move(handler)));
    }

    template <typename Message>
    void broadcast(const typename Message::cptr& message,
        result_handler&& handler, uint64_t id=zero) NOEXCEPT
    {
        boost::asio::post(strand_,
            std::bind(&p2p::do_broadcast<Message>,
                this, message, id, std::move(handler)));
    }

    // Constructors.
    // ------------------------------------------------------------------------

    DELETE_COPY_MOVE(p2p);

    /// Construct an instance.
    p2p(const settings& settings, const logger& log) NOEXCEPT;

    /// Calls close().
    virtual ~p2p() NOEXCEPT;

    // Sequences.
    // ------------------------------------------------------------------------

    /// Invoke startup and seeding sequence.
    virtual void start(result_handler&& handler) NOEXCEPT;

    /// Run inbound and outbound sessions, call from start result handler.
    virtual void run(result_handler&& handler) NOEXCEPT;

    /// Idempotent call to block on work stop, start may be reinvoked after.
    /// Must not call concurrently or from any threadpool thread (see ~).
    /// Calling any method after close results in deadlock (threadpool joined).
    virtual void close() NOEXCEPT;

    // Subscriptions.
    // ------------------------------------------------------------------------

    /// Subscribe to connection creation (allowed before start).
    /// A call after close invokes handlers with error::subscriber_stopped.
    virtual void subscribe_connect(channel_handler&& handler,
        result_handler&& complete) NOEXCEPT;

    /// Subscribe to service stop (allowed before start).
    /// A call after close invokes handlers with error::subscriber_stopped.
    virtual void subscribe_close(stop_handler&& handler,
        stop_completer&& complete) NOEXCEPT;

    /////// Unsubscribe by passing the completion handle, true if found.
    ////virtual bool unsubscribe_connect(size_t) NOEXCEPT;
    virtual bool unsubscribe_close(size_t handle) NOEXCEPT;

    // Manual connections.
    // ------------------------------------------------------------------------

    /// Maintain a connection.
    virtual void connect(const config::endpoint& endpoint) NOEXCEPT;

    /// Maintain a connection, callback is invoked on each try.
    virtual void connect(const config::endpoint& endpoint,
        channel_handler&& handler) NOEXCEPT;

    // Properties.
    // ------------------------------------------------------------------------

    /// Get the number of addresses.
    virtual size_t address_count() const NOEXCEPT;

    /// Get the number of inbound channels.
    virtual size_t inbound_channel_count() const NOEXCEPT;

    /// Get the number of channels.
    virtual size_t channel_count() const NOEXCEPT;

    /// Network configuration settings.
    const settings& network_settings() const NOEXCEPT;

    /// Return a reference to the network io_context (thread safe).
    asio::io_context& service() NOEXCEPT;

    /// Return a reference to the network strand (thread safe).
    asio::strand& strand() NOEXCEPT;

protected:
    friend class session;

    /// Attach session to network, caller must start (requires strand).
    template <class Session, typename... Args>
    typename Session::ptr attach(p2p& net, Args&&... args) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "subscribe_close");

        // Sessions are attached after network start.
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        const auto session = std::make_shared<Session>(net, ++stopper_,
            std::forward<Args>(args)...);
        BC_POP_WARNING()

        // Session lifetime is ensured by the network stop subscriber.
        subscribe_close([=](const code&) NOEXCEPT
        {
            session->stop();
            return false;
        }, stopper_);

        return session;
    }

    /// Override to attach specialized sessions.
    virtual session_seed::ptr attach_seed_session() NOEXCEPT;
    virtual session_manual::ptr attach_manual_session() NOEXCEPT;
    virtual session_inbound::ptr attach_inbound_session() NOEXCEPT;
    virtual session_outbound::ptr attach_outbound_session() NOEXCEPT;

    /// Override for test injection.
    virtual acceptor::ptr create_acceptor() NOEXCEPT;
    virtual connector::ptr create_connector() NOEXCEPT;

    /// Register nonces for loopback detection (true implies found).
    virtual bool store_nonce(uint64_t nonce) NOEXCEPT;
    virtual bool unstore_nonce(uint64_t nonce) NOEXCEPT;

    /// Register channels for broadcast and quick stop.
    virtual code store_channel(const channel::ptr& channel, bool notify,
        bool inbound) NOEXCEPT;
    virtual code unstore_channel(const channel::ptr& channel,
        bool inbound) NOEXCEPT;

    /// Maintain address pool.
    virtual void take(address_item_handler&& handler) NOEXCEPT;
    virtual void restore(const address_item_cptr& address,
        result_handler&& complete) NOEXCEPT;
    virtual void fetch(address_handler&& handler) const NOEXCEPT;
    virtual void save(const address_cptr& message,
        count_handler&& complete) NOEXCEPT;

    /// The strand is running in this thread.
    bool stranded() const NOEXCEPT;

private:
    typedef subscriber<const channel::ptr&> channel_subscriber;

    template <typename Message>
    void do_broadcast(const typename Message::cptr& message, uint64_t nonce,
        const result_handler& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        // Exclude the self channel (nonces are non-zero, so zero implies all).
        const auto self = [nonce](const auto& channel) NOEXCEPT
        {
            return channel->nonce() == nonce;
        };

        // TODO: Serialization may not be unique per channel (by version).
        // TODO: Specialize this template for messages unique by version.
        // Serialization is here to preclude serialization in each channel.
        const auto data = messages::serialize(message, settings_.identifier,
            /*channel->version()*/ messages::level::canonical);

        for (const auto& channel: channels_)
            if (!self(channel))
                channel->write(data, move_copy(handler));
    }

    bool subscribe_close(stop_handler&& handler, const stop_key& key) NOEXCEPT;
    connectors_ptr create_connectors(size_t count) NOEXCEPT;

    virtual bool closed() const NOEXCEPT;
    virtual code start_hosts() NOEXCEPT;
    virtual code stop_hosts() NOEXCEPT;

    void do_start(const result_handler& handler) NOEXCEPT;
    void do_run(const result_handler& handler) NOEXCEPT;
    void do_close() NOEXCEPT;

    void handle_start(const code& ec, const result_handler& handler) NOEXCEPT;
    void handle_run(const code& ec, const result_handler& handler) NOEXCEPT;
  
    void do_subscribe_connect(const channel_handler& handler,
        const result_handler& complete) NOEXCEPT;
    void do_subscribe_close(const stop_handler& handler,
        const stop_completer& complete) NOEXCEPT;

    // Distinct method names required for std::bind.
    void do_connect(const config::endpoint& endpoint) NOEXCEPT;
    void do_connect_handled(const config::endpoint& endpoint,
        const channel_handler& handler) NOEXCEPT;

    void do_take(const address_item_handler& handler) NOEXCEPT;
    void do_restore(const address_item_cptr& host,
        const result_handler& complete) NOEXCEPT;
    void do_fetch(const address_handler& handler) const NOEXCEPT;
    void do_save(const address_cptr& message,
        const count_handler& complete) NOEXCEPT;

    // These are thread safe.
    const settings& settings_;
    std::atomic<size_t> channel_count_{};
    std::atomic<size_t> inbound_channel_count_{};

    // These are protected by strand.
    hosts hosts_;
    threadpool threadpool_;
    session_manual::ptr manual_{};

    // This is thread safe.
    asio::strand strand_;

    // These are protected by strand.

    stop_key stopper_{};
    stop_subscriber stop_subscriber_;

    size_t connecter_{};
    channel_subscriber connect_subscriber_;

    std::unordered_set<uint64_t> nonces_{};
    std::unordered_set<channel::ptr> channels_{};
    std::unordered_set<config::authority> authorities_{};
};

} // namespace network
} // namespace libbitcoin

#endif
