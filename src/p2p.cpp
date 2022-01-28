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
#include <bitcoin/network/p2p.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <utility>
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

using namespace bc::system;
using namespace bc::system::chain;
using namespace std::placeholders;

// This can be exceeded due to manual connection calls and race conditions.
inline size_t nominal_connecting(const settings& settings)
{
    return settings.peers.size() + settings.connect_batch_size *
        settings.outbound_connections;
}

// This can be exceeded due to manual connection calls and race conditions.
inline size_t nominal_connected(const settings& settings)
{
    return settings.peers.size() + settings.outbound_connections +
        settings.inbound_connections;
}

p2p::p2p(const settings& settings)
  : settings_(settings),
    stopped_(true),
    hosts_(settings_),
    manual_(nullptr),
    threadpool_(settings_.threads),
    strand_(threadpool_.service().get_executor()),
    stop_subscriber_(std::make_shared<stop_subscriber>(strand_)),
    channel_subscriber_(std::make_shared<channel_subscriber>(strand_))
{
}

p2p::~p2p()
{
    p2p::do_stop();
}

// Start sequence.
// ----------------------------------------------------------------------------

void p2p::start(result_handler handler)
{
    const auto ptr = to_shared<messages::address>(new messages::address);

    if (!stopped())
    {
        handler(error::operation_failed);
        return;
    }

    stopped_.store(false, std::memory_order_relaxed);

    // The instance is retained by p2p stop handler (until shutdown).
    // The member reference is retained for posting manual connect calls.
    manual_ = attach_manual_session();
    manual_->start(std::bind(&p2p::handle_manual_started, this, _1, handler));
}

void p2p::handle_manual_started(const code& ec, result_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    if (ec)
    {
        handler(ec);
        return;
    }

    handle_hosts_loaded(hosts_.start(), handler);
}

void p2p::handle_hosts_loaded(const code& ec, result_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    if (ec)
    {
        handler(ec);
        return;
    }

    // Subscription for seed node connections is not supported.
    // The instance is retained by p2p stop handler (until shutdown).
    const auto seed = attach_seed_session();
    seed->start(std::bind(&p2p::handle_started, this, _1, handler));
}

// The seed session is complete upon the invocation of its start handler.
void p2p::handle_started(const code& ec, result_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    if (ec)
    {
        handler(ec);
        return;
    }

    // This is the end of the start sequence.
    handler(error::success);
}

// Run sequence.
// ----------------------------------------------------------------------------

void p2p::run(result_handler handler)
{
    // Start configured persistent connections.
    for (const auto& peer: settings_.peers)
        connect(peer);

    // The instance is retained by the stop handler (until shutdown).
    const auto inbound = attach_inbound_session();
    inbound->start(std::bind(&p2p::handle_inbound_started, this, _1, handler));
}

void p2p::handle_inbound_started(const code& ec,
    result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    // The instance is retained by the stop handler (until shutdown).
    const auto outbound = attach_outbound_session();
    outbound->start(std::bind(&p2p::handle_running, this, _1, handler));
}

void p2p::handle_running(const code& ec, result_handler handler)
{
    if (ec)
    {
        handler(ec);
        return;
    }

    // This is the end of the run sequence.
    handler(error::success);
}

// Shutdown sequence.
// ----------------------------------------------------------------------------
// p2p must be kept in scope until stop hander returns or undefined behavior.

// Blocks on join of all threadpool threads.
// Must not be called from a thread within threadpool_.
void p2p::stop()
{
    stopped_.store(true, std::memory_order_relaxed);
    boost::asio::post(strand_, std::bind(&p2p::do_stop, this));
    threadpool_.join();
}

void p2p::do_stop()
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Stop and clear manual session.
    manual_->stop(error::service_stopped);
    manual_.reset();

    // Stop and clear all other sessions.
    stop_subscriber_->stop(error::service_stopped);

    // Clear subscribers to new channel notifications.
    channel_subscriber_->stop(error::service_stopped, nullptr);

    // Stop and clear all channels.
    // These are each posted to each channel strand by the channel proxy.
    // Each proxy stop subscriber will invoke stop handlers on that strand.
    // That causes session channel removal to be posted to network strand.
    for (const auto& channel: channels_)
        channel->stop(error::service_stopped);

    // Serialize hosts file (log results).
    /*code*/ hosts_.stop();

    // Stop threadpool keep-alive, all work must self-terminate to affect join.
    threadpool_.stop();
}

bool p2p::stopped() const
{
    return stopped_.load(std::memory_order_relaxed);
}

// Subscriptions.
// ----------------------------------------------------------------------------

// External or derived callers.
void p2p::subscribe_connect(connect_handler handler)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_subscribe_connect, this, std::move(handler)));
}

void p2p::do_subscribe_connect(connect_handler handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    channel_subscriber_->subscribe(handler);
}

// Sessions subscribe to network close.
void p2p::subscribe_close(result_handler handler)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_subscribe_close, this, std::move(handler)));
}

void p2p::do_subscribe_close(result_handler handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    stop_subscriber_->subscribe(handler);
}

// Manual connections (do not invoke mutiple connect concurrently).
// ----------------------------------------------------------------------------

void p2p::connect(const config::endpoint& endpoint)
{
    manual_->connect(endpoint.host(), endpoint.port());
}

void p2p::connect(const std::string& hostname, uint16_t port)
{
    if (stopped())
        return;

    manual_->connect(hostname, port);
}

// Handler is invoked after handshake and before protocol attachment.
void p2p::connect(const std::string& hostname, uint16_t port,
    channel_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    manual_->connect(hostname, port, handler);
}

// Properties.
// ----------------------------------------------------------------------------

const settings& p2p::network_settings() const
{
    return settings_;
}

asio::io_context& p2p::service()
{
    return threadpool_.service();
}

asio::strand& p2p::strand()
{
    return strand_;
}

// Specializations (protected).
// ----------------------------------------------------------------------------
// Create derived sessions and override these to inject from derived p2p class.

session_seed::ptr p2p::attach_seed_session()
{
    return attach<session_seed>();
}

session_manual::ptr p2p::attach_manual_session()
{
    return attach<session_manual>();
}

session_inbound::ptr p2p::attach_inbound_session()
{
    return attach<session_inbound>();
}

session_outbound::ptr p2p::attach_outbound_session()
{
    return attach<session_outbound>();
}

// Hosts collection.
// ----------------------------------------------------------------------------

// Thread safe.
size_t p2p::address_count() const
{
    return hosts_.count();
}

void p2p::load(const messages::address_item& host)
{
    boost::asio::post(strand_, std::bind(&p2p::do_load, this, host));
}

void p2p::do_load(const messages::address_item& host)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.store(host);
}

// TODO: use pointer.
void p2p::load(const messages::address_items& hosts)
{
    boost::asio::post(strand_, std::bind(&p2p::do_loads, this, hosts));
}

void p2p::do_loads(const messages::address_items& hosts)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.store(hosts);
}

void p2p::unload(const messages::address_item& host)
{
    boost::asio::post(strand_, std::bind(&p2p::do_unload, this, host));
}

void p2p::do_unload(const messages::address_item& host)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.remove(host);
}

void p2p::fetch_address(hosts::peer_handler handler) const
{
    boost::asio::post(
        strand_, std::bind(&p2p::do_fetch_address, this, std::move(handler)));
}

void p2p::do_fetch_address(hosts::peer_handler handler) const
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.fetch(handler);
}

void p2p::fetch_addresses(hosts::peers_handler handler) const
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_fetch_addresses, this, std::move(handler)));
}

void p2p::do_fetch_addresses(hosts::peers_handler handler) const
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.fetch(handler);
}

////// Connection management.
////// ----------------------------------------------------------------------------
////// TODO: Move to session base.
////
////code p2p::pend(connector::ptr connector)
////{
////    return pending_connect_.store(connector);
////}
////
////void p2p::unpend(connector::ptr connector)
////{
////    // TODO: remove code.
////    connector->stop(error::success);
////    pending_connect_.remove(connector);
////}

// Thread safe, inexact (ok).
size_t p2p::channel_count() const
{
    return channel_count_.load(std::memory_order_relaxed);
}

void p2p::pend(uint64_t nonce)
{
    boost::asio::post(strand_, std::bind(&p2p::do_pend, this, nonce));
}

void p2p::do_pend(uint64_t nonce)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    nonces_.insert(nonce);
}

void p2p::unpend(uint64_t nonce)
{
    boost::asio::post(strand_, std::bind(&p2p::do_unpend, this, nonce));
}

void p2p::do_unpend(uint64_t nonce)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    nonces_.erase(nonce);
}

void p2p::store(channel::ptr channel, bool notify, bool inbound,
    result_handler handler)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_store, this, channel, notify, inbound, handler));
}

// Channel is presumed to be started.
void p2p::do_store(channel::ptr channel, bool notify, bool inbound,
    result_handler handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    // Cannot allow any store once stopped, or do_stop will not free it.
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    // Check for connection to self (but only on incoming).
    if (inbound &&
        nonces_.find(channel->peer_version()->nonce) != nonces_.end())
    {
        handler(error::accept_failed);
        return;
    }

    // Store the peer address, exit if already exists.
    if (!addresses_.insert(channel->authority()).second)
    {
        handler(error::address_in_use);
        return;
    }

    // Notify channel subscribers of started channel.
    if (notify)
        channel_subscriber_->notify(error::success, channel);

    ++channel_count_;
    channels_.insert(channel);
    handler(error::success);
}

void p2p::unstore(channel::ptr channel)
{
    boost::asio::post(strand_, std::bind(&p2p::do_unstore, this, channel));
}

// Channel is presumed to be stopped.
void p2p::do_unstore(channel::ptr channel)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    --channel_count_;
    channels_.erase(channel);
    addresses_.erase(channel->authority());
}

} // namespace network
} // namespace libbitcoin
