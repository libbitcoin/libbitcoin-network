/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/net.hpp>

#include <algorithm>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace system;
using namespace std::placeholders;

net::net(const settings& settings, const logger& log) NOEXCEPT
  : settings_(settings),
    threadpool_(std::max(settings.threads, 1_u32)),
    strand_(threadpool_.service().get_executor()),
    hosts_(settings, log),
    broadcaster_(strand_),
    stop_subscriber_(strand_),
    connect_subscriber_(strand_),
    reporter(log)
{
    ////LOG_LOG("Aplication log compiled..: ", news_defined);
    ////LOG_LOG("News log compiled........: ", news_defined);
    ////LOG_LOG("Session log compiled.....: ", session_defined);
    ////LOG_LOG("Protocol log compiled....: ", protocol_defined);
    ////LOG_LOG("ProXy log compiled.......: ", proxy_defined);
    ////LOG_LOG("Remote log compiled......: ", remote_defined);
    ////LOG_LOG("Fault log compiled.......: ", fault_defined);
    ////LOG_LOG("Quitting log compiled....: ", quitting_defined);
    ////LOG_LOG("Object log compiled......: ", objects_defined);
    ////LOG_LOG("Verbose log compiled.....: ", verbose_defined);
}

net::~net() NOEXCEPT
{
    // Weak references in threadpool closures safe as net joins threads here.
    net::close();
}

// I/O factories.
// ----------------------------------------------------------------------------

acceptor::ptr net::create_acceptor() NOEXCEPT
{
    return std::make_shared<acceptor>(log, strand(), service(),
        network_settings(), accept_suspended_);
}

connector::ptr net::create_connector() NOEXCEPT
{
    return std::make_shared<connector>(log, strand(), service(),
        network_settings(), connect_suspended_);
}

connectors_ptr net::create_connectors(size_t count) NOEXCEPT
{
    const auto connects = std::make_shared<connectors>();
    connects->reserve(count);

    for (size_t connect = 0; connect < count; ++connect)
        connects->push_back(create_connector());

    return connects;
}

// Start sequence.
// ----------------------------------------------------------------------------

void net::start(result_handler&& handler) NOEXCEPT
{
    // Threadpool is started on construct, stopped only by work starvation.
    boost::asio::post(strand_,
        std::bind(&net::do_start, this, std::move(handler)));
}

void net::do_start(const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    manual_ = attach_manual_session();
    manual_->start(std::bind(&net::handle_start, this, _1, handler));
}

void net::handle_start(const code& ec, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        handler(ec);
        return;
    }

    // Deserialize hosts from file.
    if (const auto error_code = start_hosts())
    {
        LOGF("Hosts file failed to deserialize, " << error_code.message());
        handler(error_code);
        return;
    }

    attach_seed_session()->start(move_copy(handler));
}

// Run sequence (seeding may be ongoing after its handler is invoked).
// ----------------------------------------------------------------------------

void net::run(result_handler&& handler) NOEXCEPT
{
    // Public methods can complete on caller thread.
    if (closed())
    {
        handler(error::service_stopped);
        return;
    }

    boost::asio::post(strand_,
        std::bind(&net::do_run, this, std::move(handler)));
}

void net::do_run(const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (closed())
    {
        handler(error::service_stopped);
        return;
    }

    for (const auto& peer: settings_.peers)
        do_connect(peer);

    attach_inbound_session()->start(
        std::bind(&net::handle_run, this, _1, handler));
}

void net::handle_run(const code& ec, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        handler(ec);
        return;
    }

    attach_outbound_session()->start(
        std::bind(&net::handle_client, this, _1, handler));
}

void net::handle_client(const code& ec, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        handler(ec);
        return;
    }

    attach_client_session()->start(move_copy(handler));
}

// Shutdown sequence.
// ----------------------------------------------------------------------------

// Results in std::abort if called from a thread within the threadpool.
void net::close() NOEXCEPT
{
    closed_.store(true);
    boost::asio::post(strand_,
        std::bind(&net::do_close, this));

    // Blocks on join of all threadpool threads.
    if (!threadpool_.join())
    {
        BC_ASSERT_MSG(false, "failed to join threadpool");
        std::abort();
    }

    // Serialize hosts to file.
    if (const auto error_code = stop_hosts())
    {
        LOGF("Hosts file failed to serialize, " << error_code.message());
    }
}

void net::do_close() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Release reference to manual session (also held by stop subscriber).
    if (manual_) manual_.reset();

    // Notify and delete all stop subscribers (all sessions).
    stop_subscriber_.stop(error::service_stopped);

    // Notify and delete subscribers to channel notifications.
    connect_subscriber_.stop_default(error::service_stopped);

    // Notify and delete subscribers to message broadcast notifications.
    broadcaster_.stop(error::service_stopped);

    // Stop threadpool keep-alive, all work must self-terminate to affect join.
    threadpool_.stop();
}

bool net::closed() const NOEXCEPT
{
    return closed_.load();
}

// Suspensions.
// ----------------------------------------------------------------------------

void net::suspend(const code&) NOEXCEPT
{
    suspend_acceptors();
    suspend_connectors();
}

void net::resume() NOEXCEPT
{
    resume_acceptors();
    resume_connectors();
}

bool net::suspended() const NOEXCEPT
{
    return connect_suspended_.load() || accept_suspended_.load();
}

void net::suspend_acceptors() NOEXCEPT
{
    accept_suspended_.store(true);
}

void net::resume_acceptors() NOEXCEPT
{
    accept_suspended_.store(false);
}

void net::suspend_connectors() NOEXCEPT
{
    connect_suspended_.store(true);
}

void net::resume_connectors() NOEXCEPT
{
    connect_suspended_.store(false);
}

// Properties.
// ----------------------------------------------------------------------------

const settings& net::network_settings() const NOEXCEPT
{
    return settings_;
}

asio::io_context& net::service() NOEXCEPT
{
    return threadpool_.service();
}

asio::strand& net::strand() NOEXCEPT
{
    return strand_;
}

bool net::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}


// Subscriptions.
// ----------------------------------------------------------------------------
// Channel and network strands share same pool, and as long as a job is
// running in the pool, it will continue to accept work. Therefore handlers
// will not be orphaned during a stop as long as they remain in the pool.
// But when entering from outside the pool (such as subscribe) handler must be
// invoked when stopped as the handler will go uninvoked if the pool empties.

// public
void net::subscribe_connect(channel_notifier&& handler,
    channel_completer&& complete) NOEXCEPT
{
    // Public methods can complete on caller thread.
    if (closed())
    {
        complete(error::service_stopped, {});
        handler(error::service_stopped, {});
        return;
    }

    boost::asio::post(strand_,
        std::bind(&net::do_subscribe_connect,
            this, std::move(handler), std::move(complete)));
}

void net::do_subscribe_connect(const channel_notifier& handler,
    const channel_completer& complete) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    const auto key = create_key();
    complete(connect_subscriber_.subscribe(move_copy(handler), key), key);
}

// protected
void net::notify_connect(const channel::ptr& channel) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&net::do_notify_connect, this, channel));
}

void net::do_notify_connect(const channel::ptr& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    connect_subscriber_.notify(error::success, channel);
}

void net::unsubscribe_connect(object_key key) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&net::do_unsubscribe_connect, this, key));
}

void net::do_unsubscribe_connect(object_key key) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    connect_subscriber_.notify_one(key, error::desubscribed, nullptr);
}

// protected
void net::subscribe_close(stop_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    subscribe_close(std::move(handler), create_key());
}

// private
code net::subscribe_close(stop_handler&& handler, object_key key) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return stop_subscriber_.subscribe(std::move(handler), key);
}

// public
void net::subscribe_close(stop_handler&& handler,
    stop_completer&& complete) NOEXCEPT
{
    // Public methods can complete on caller thread.
    if (closed())
    {
        complete(error::service_stopped, {});
        handler(error::service_stopped);
        return;
    }

    boost::asio::post(strand_,
        std::bind(&net::do_subscribe_close,
            this, std::move(handler), std::move(complete)));
}

void net::do_subscribe_close(const stop_handler& handler,
    const stop_completer& complete) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    
    const auto key = create_key();
    complete(subscribe_close(move_copy(handler), key), key);
}

void net::unsubscribe_close(object_key key) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&net::do_unsubscribe_close, this, key));
}

void net::do_unsubscribe_close(object_key key) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_.notify_one(key, error::desubscribed);
}

// At one object/session/ns, this overflows in ~585 years (and handled).
net::object_key net::create_key() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (is_zero(++keys_))
    {
        BC_ASSERT_MSG(false, "overflow");
        LOGF("Session object overflow.");
    }

    return keys_;
}

// P2P Manual connections.
// ----------------------------------------------------------------------------

void net::connect(const config::endpoint& endpoint) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&net::do_connect, this, endpoint));
}

void net::do_connect(const config::endpoint& endpoint) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    if (manual_) manual_->connect(endpoint);
}

void net::connect(const config::endpoint& endpoint,
    channel_notifier&& handler) NOEXCEPT
{
    // Public methods can complete on caller thread.
    if (closed())
    {
        handler(error::service_stopped, {});
        return;
    }

    boost::asio::post(strand_,
        std::bind(&net::do_connect_handled, this, endpoint,
            std::move(handler)));
}

void net::do_connect_handled(const config::endpoint& endpoint,
    const channel_notifier& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (manual_)
        manual_->connect(endpoint, move_copy(handler));
    else
        handler(error::service_stopped, nullptr);
}

// P2P Properties.
// ----------------------------------------------------------------------------

size_t net::address_count() const NOEXCEPT
{
    return hosts_.count();
}

size_t net::reserved_count() const NOEXCEPT
{
    return hosts_.reserved();
}

size_t net::channel_count() const NOEXCEPT
{
    return total_channel_count_;
}

size_t net::inbound_channel_count() const NOEXCEPT
{
    return inbound_channel_count_;
}

// P2P hosts collection.
// ----------------------------------------------------------------------------
// Protected, called from session (network strand) and channel (network pool).

// private
code net::start_hosts() NOEXCEPT
{
    return hosts_.start();
}

// private
code net::stop_hosts() NOEXCEPT
{
    return hosts_.stop();
}

void net::take(address_item_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&net::do_take, this, std::move(handler)));
}

void net::do_take(const address_item_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    hosts_.take(move_copy(handler));
}

void net::restore(const address_item_cptr& address,
    result_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&net::do_restore, this, address, std::move(handler)));
}

void net::do_restore(const address_item_cptr& address,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    hosts_.restore(address, move_copy(handler));
}

void net::fetch(address_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&net::do_fetch, this, std::move(handler)));
}

void net::do_fetch(const address_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Accelerate stop, since hosts keeps running until all threads closed.
    if (closed())
    {
        handler(error::service_stopped, {});
        return;
    }

    hosts_.fetch(move_copy(handler));
}

void net::save(const address_cptr& message, count_handler&& handler) NOEXCEPT
{
    boost::asio::post(strand_,
        std::bind(&net::do_save, this, message, std::move(handler)));
}

void net::do_save(const address_cptr& message,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Accelerate stop, since hosts keeps running until all threads closed.
    if (closed())
    {
        handler(error::service_stopped, {});
        return;
    }

    hosts_.save(message, move_copy(handler));
}

// P2P loopback detection.
// ----------------------------------------------------------------------------

bool net::store_nonce(const channel_peer& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (settings_.enable_loopback || channel.inbound())
        return true;

    if (!nonces_.insert(channel.nonce()).second)
    {
        LOGF("Failed to store nonce for [" << channel.authority() << "].");
        return false;
    }

    return true;
}

bool net::unstore_nonce(const channel_peer& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (settings_.enable_loopback || channel.inbound())
        return true;

    if (!to_bool(nonces_.erase(channel.nonce())))
    {
        LOGF("Failed to unstore nonce for [" << channel.authority() << "].");
        return false;
    }

    return true;
}

bool net::is_loopback(const channel_peer& channel) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (settings_.enable_loopback || !channel.inbound())
        return false;

    return to_bool(nonces_.count(channel.peer_version()->nonce));
}

// P2P channel counting with address deconfliction.
// ----------------------------------------------------------------------------

code net::count_channel(const channel_peer& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (closed())
        return error::service_stopped;

    if (is_loopback(channel))
    {
        LOGS("Loopback detected from [" << channel.authority() << "].");
        return error::accept_failed;
    }

    if (channel.inbound() && is_zero(add1(inbound_channel_count_.load())))
    {
        LOGF("Overflow: inbound channel count.");
        return error::channel_overflow;
    }

    if (!channel.quiet() && is_zero(add1(total_channel_count_.load())))
    {
        LOGF("Overflow: total channel count.");
        return error::channel_overflow;
    }

    if (!hosts_.reserve(channel.authority()))
    {
        LOGS("Duplicate connection to [" << channel.authority() << "].");
        return error::address_in_use;
    }

    if (channel.inbound())
        ++inbound_channel_count_;

    if (!channel.quiet())
        ++total_channel_count_;

    return error::success;
}

void net::uncount_channel(const channel_peer& channel) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    hosts_.unreserve(channel.authority());

    if (channel.inbound() && is_zero(inbound_channel_count_.load()))
    {
        LOGF("Underflow: inbound channel count.");
        return;
    }

    if (!channel.quiet() && is_zero(total_channel_count_.load()))
    {
        LOGF("Underflow: total channel count.");
        return;
    }

    if (channel.inbound())
        --inbound_channel_count_;

    if (!channel.quiet())
        --total_channel_count_;
}

// Specializations (protected).
// ----------------------------------------------------------------------------

session_seed::ptr net::attach_seed_session() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return attach<session_seed>(*this);
}

session_manual::ptr net::attach_manual_session() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return attach<session_manual>(*this);
}

session_inbound::ptr net::attach_inbound_session() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return attach<session_inbound>(*this);
}

session_outbound::ptr net::attach_outbound_session() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return attach<session_outbound>(*this);
}

session_inbound_client::ptr net::attach_client_session() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return attach<session_inbound_client>(*this);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
