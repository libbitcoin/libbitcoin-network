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
#ifndef LIBBITCOIN_NETWORK_P2P_HPP
#define LIBBITCOIN_NETWORK_P2P_HPP

#include <atomic>
#include <functional>
#include <memory>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// Peer-to-Peer network class, virtual, thread safe with exceptions:
/// * attach must be called from network strand.
/// * close must not be called concurrently or from any threadpool thread.
class BCT_API p2p
  : public reporter
{
public:
    typedef std::shared_ptr<p2p> ptr;
    typedef uint64_t object_key;

    typedef desubscriber<object_key> stop_subscriber;
    typedef stop_subscriber::handler stop_handler;
    typedef stop_subscriber::completer stop_completer;

    typedef desubscriber<object_key, const channel::ptr&> channel_subscriber;
    typedef channel_subscriber::handler channel_notifier;
    typedef channel_subscriber::completer channel_completer;

    /// Constructors.
    /// -----------------------------------------------------------------------

    DELETE_COPY_MOVE(p2p);

    /// Construct an instance.
    p2p(const settings& settings, const logger& log) NOEXCEPT;

    /// Calls close().
    virtual ~p2p() NOEXCEPT;

    /// Sequences.
    /// -----------------------------------------------------------------------

    /// Invoke startup and seeding sequence, not thread safe or restartable.
    virtual void start(result_handler&& handler) NOEXCEPT;

    /// Run inbound and outbound sessions, call from start result handler.
    virtual void run(result_handler&& handler) NOEXCEPT;

    /// Idempotent call to block on work stop.
    /// Must not call concurrently or from any threadpool thread (see ~).
    virtual void close() NOEXCEPT;

    /// Subscriptions.
    /// -----------------------------------------------------------------------
    /// A channel pointer should only be retained when subscribed to its stop,
    /// and must be unretained in stop handler invoke, otherwise it will leak.
    /// To subscribe to disconnections, subscribe to each channel stop.
    /// Subscriptions and unsubscriptions are allowed before start.

    /// Subscribe to connection creation.
    /// A call after close invokes handlers with error::subscriber_stopped.
    virtual void subscribe_connect(channel_notifier&& handler,
        channel_completer&& complete) NOEXCEPT;

    /// Subscribe to service stop.
    /// A call after close invokes handlers with error::subscriber_stopped.
    virtual void subscribe_close(stop_handler&& handler,
        stop_completer&& complete) NOEXCEPT;

    /// Unsubscribe by subscription key, error::desubscribed passed to handler.
    virtual void unsubscribe_connect(object_key key) NOEXCEPT;
    virtual void unsubscribe_close(object_key key) NOEXCEPT;

    /// Manual connections.
    /// -----------------------------------------------------------------------

    /// Maintain a connection.
    virtual void connect(const config::endpoint& endpoint) NOEXCEPT;

    /// Maintain a connection, callback is invoked on each try.
    virtual void connect(const config::endpoint& endpoint,
        channel_notifier&& handler) NOEXCEPT;

    /// Suspensions.
    /// -----------------------------------------------------------------------

    /// Network connections are suspended (incoming and/or outgoing).
    virtual bool suspended() const NOEXCEPT;

    /// Suspend all connections.
    virtual void suspend(const code& ec) NOEXCEPT;

    /// Resume all connection.
    virtual void resume() NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The node threadpool is stopped and may still be joining.
    virtual bool closed() const NOEXCEPT;

    /// Get the number of addresses.
    virtual size_t address_count() const NOEXCEPT;

    /// Get the number of address reservations.
    virtual size_t reserved_count() const NOEXCEPT;

    /// Get the number of channels.
    virtual size_t channel_count() const NOEXCEPT;

    /// Get the number of inbound channels.
    virtual size_t inbound_channel_count() const NOEXCEPT;

    /// Network configuration settings.
    const settings& network_settings() const NOEXCEPT;

    /// Return a reference to the network io_context (thread safe).
    asio::io_context& service() NOEXCEPT;

    /// Return a reference to the network strand (thread safe).
    asio::strand& strand() NOEXCEPT;

    /// The strand is running in this thread.
    bool stranded() const NOEXCEPT;

    /// TEMP HACKS.
    /// -----------------------------------------------------------------------
    /// Not thread safe, read from stranded handler only.

    virtual size_t stop_subscriber_count() const NOEXCEPT
    {
        return stop_subscriber_.size();
    }

    virtual size_t connect_subscriber_count() const NOEXCEPT
    {
        return connect_subscriber_.size();
    }

    virtual size_t nonces_count() const NOEXCEPT
    {
        return nonces_.size();
    }

protected:
    friend class session;

    /// Attach session to network, caller must start (requires strand).
    template <class Session, class Network, typename... Args>
    typename Session::ptr attach(Network& net, Args&&... args) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "subscribe_close");
        const auto id = create_key();

        // Sessions are attached after network start.
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        const auto session = std::make_shared<Session>(net, id,
            std::forward<Args>(args)...);
        BC_POP_WARNING()

        // Session lifetime is ensured by the network stop subscriber.
        subscribe_close([=](const code&) NOEXCEPT
        {
            session->stop();
            return false;
        }, id);

        return session;
    }

    virtual void do_start(const result_handler& handler) NOEXCEPT;
    virtual void do_run(const result_handler& handler) NOEXCEPT;
    virtual void do_close() NOEXCEPT;

    /// Override to attach specialized sessions, require strand.
    virtual session_seed::ptr attach_seed_session() NOEXCEPT;
    virtual session_manual::ptr attach_manual_session() NOEXCEPT;
    virtual session_inbound::ptr attach_inbound_session() NOEXCEPT;
    virtual session_outbound::ptr attach_outbound_session() NOEXCEPT;

    /// Override for test injection.
    virtual acceptor::ptr create_acceptor() NOEXCEPT;
    virtual connector::ptr create_connector() NOEXCEPT;

    /// Register nonces for loopback (true implies found), require strand.
    virtual bool store_nonce(const channel& channel) NOEXCEPT;
    virtual bool unstore_nonce(const channel& channel) NOEXCEPT;
    virtual bool is_loopback(const channel& channel) const NOEXCEPT;

    /// Count channel, guard loopback, reserve address.
    virtual code count_channel(const channel& channel) NOEXCEPT;
    virtual void uncount_channel(const channel& channel) NOEXCEPT;

    /// Notify subscribers of new non-seed connection, require strand.
    virtual void notify_connect(const channel::ptr& channel) NOEXCEPT;
    virtual void subscribe_close(stop_handler&& handler) NOEXCEPT;

    /// Maintain address pool.
    virtual void take(address_item_handler&& handler) NOEXCEPT;
    virtual void restore(const address_item_cptr& address,
        result_handler&& complete) NOEXCEPT;
    virtual void fetch(address_handler&& handler) NOEXCEPT;
    virtual void save(const address_cptr& message,
        count_handler&& complete) NOEXCEPT;

private:
    code subscribe_close(stop_handler&& handler, object_key key) NOEXCEPT;
    connectors_ptr create_connectors(size_t count) NOEXCEPT;
    object_key create_key() NOEXCEPT;

    virtual code start_hosts() NOEXCEPT;
    virtual code stop_hosts() NOEXCEPT;

    /// Suspend/resume inbound/outbound connections.
    void suspend_acceptors() NOEXCEPT;
    void resume_acceptors() NOEXCEPT;
    void suspend_connectors() NOEXCEPT;
    void resume_connectors() NOEXCEPT;

    void handle_start(const code& ec, const result_handler& handler) NOEXCEPT;
    void handle_run(const code& ec, const result_handler& handler) NOEXCEPT;

    void do_unsubscribe_connect(object_key key) NOEXCEPT;
    void do_notify_connect(const channel::ptr& channel) NOEXCEPT;
    void do_subscribe_connect(const channel_notifier& handler,
        const channel_completer& complete) NOEXCEPT;

    void do_unsubscribe_close(object_key key) NOEXCEPT;
    void do_subscribe_close(const stop_handler& handler,
        const stop_completer& complete) NOEXCEPT;

    void do_connect(const config::endpoint& endpoint) NOEXCEPT;
    void do_connect_handled(const config::endpoint& endpoint,
        const channel_notifier& handler) NOEXCEPT;

    void do_take(const address_item_handler& handler) NOEXCEPT;
    void do_restore(const address_item_cptr& address,
        const result_handler& handler) NOEXCEPT;
    void do_fetch(const address_handler& handler) NOEXCEPT;
    void do_save(const address_cptr& message,
        const count_handler& handler) NOEXCEPT;

    // These are thread safe.
    const settings& settings_;
    std::atomic_bool closed_{ false };
    std::atomic_bool accept_suspended_{ false };
    std::atomic_bool connect_suspended_{ false };
    std::atomic<size_t> total_channel_count_{};
    std::atomic<size_t> inbound_channel_count_{};

    // These are protected by strand.
    session_manual::ptr manual_{};
    threadpool threadpool_;

    // This is thread safe.
    asio::strand strand_;

    // These are protected by strand.
    hosts hosts_;
    broadcaster broadcaster_;
    stop_subscriber stop_subscriber_;
    channel_subscriber connect_subscriber_;
    object_key keys_{};

    // Guards loopback.
    std::unordered_set<uint64_t> nonces_{};
};

} // namespace network
} // namespace libbitcoin

#endif
