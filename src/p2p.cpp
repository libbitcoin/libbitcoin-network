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
    channel_count_(zero),
    hosts_(settings_),
    threadpool_(settings_.threads),
    strand_(threadpool_.service().get_executor()),
    stop_subscriber_(std::make_shared<stop_subscriber>(strand_)),
    channel_subscriber_(std::make_shared<channel_subscriber>(strand_))
{
}

p2p::~p2p()
{
    p2p::stop();
}

// Start sequence.
// ----------------------------------------------------------------------------

void p2p::start(result_handler handler)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_start, this, std::move(handler)));
}

// private
void p2p::do_start(result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "attach_manual_session");

    manual_ = attach_manual_session();
    manual_->start(
        boost::asio::bind_executor(strand_,
            std::bind(&p2p::handle_start, this, _1, handler)));
}

// private
void p2p::handle_start(code ec, result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "hosts_");

    if (ec || ((ec = hosts_.start())))
    {
        handler(ec);
        return;
    }

    attach_seed_session()->start(handler);
}

// Run sequence.
// ----------------------------------------------------------------------------

void p2p::run(result_handler handler)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_run, this, std::move(handler)));
}

// private
void p2p::do_run(result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "manual_, attach_inbound_session");

    if (!manual_)
    {
        handler(error::service_stopped);
        return;
    }

    for (const auto& peer: settings_.peers)
        do_connect1(peer);

    attach_inbound_session()->start(
        boost::asio::bind_executor(strand_,
            std::bind(&p2p::handle_run, this, _1, std::move(handler))));
}

// private
void p2p::handle_run(code ec, result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (ec)
    {
        handler(ec);
        return;
    }

    attach_outbound_session()->start(handler);
}

// Shutdown sequence.
// ----------------------------------------------------------------------------
// p2p must be kept in scope until stop hander returns or undefined behavior.

// Not thread safe.
// Blocks on join of all threadpool threads.
// Must not be called from a thread within threadpool_.
void p2p::stop()
{
    boost::asio::post(strand_, std::bind(&p2p::do_stop, this));
    threadpool_.join();
}

// private
void p2p::do_stop()
{
    BC_ASSERT_MSG(stranded(), "do_stop (multiple members)");

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

// Subscriptions.
// ----------------------------------------------------------------------------

// External or derived callers.
void p2p::subscribe_connect(connect_handler handler,
    result_handler complete)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_subscribe_connect,
            this, std::move(handler), std::move(complete)));
}

// private
void p2p::do_subscribe_connect(connect_handler handler,
    result_handler complete)
{
    BC_ASSERT_MSG(stranded(), "channel_subscriber_");
    channel_subscriber_->subscribe(std::move(handler));
    complete(error::success);
}

void p2p::subscribe_close(result_handler handler,
    result_handler complete)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_subscribe_close,
            this, std::move(handler), std::move(complete)));
}

// Sessions subscribe to network close here via attach.
void p2p::do_subscribe_close(result_handler handler,
    result_handler complete)
{
    BC_ASSERT_MSG(stranded(), "stop_subscriber_");
    stop_subscriber_->subscribe(std::move(handler));
    complete(error::success);
}

// Manual connections.
// ----------------------------------------------------------------------------

void p2p::connect(const config::endpoint& endpoint)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_connect1, this, endpoint));
}

void p2p::connect(const std::string& hostname, uint16_t port)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_connect2, this, hostname, port));
}

void p2p::connect(const std::string& hostname, uint16_t port,
    channel_handler handler)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_connect3, this, hostname, port, std::move(handler)));
}

// private
void p2p::do_connect1(const config::endpoint& endpoint)
{
    BC_ASSERT_MSG(stranded(), "manual_");

    if (manual_)
        manual_->connect(endpoint.host(), endpoint.port());
}

// private
void p2p::do_connect2(const std::string& hostname, uint16_t port)
{
    BC_ASSERT_MSG(stranded(), "manual_");

    if (manual_)
        manual_->connect(hostname, port);
}

// private
void p2p::do_connect3(const std::string& hostname, uint16_t port,
    channel_handler handler)
{
    BC_ASSERT_MSG(stranded(), "manual_");

    if (manual_)
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

bool p2p::stranded() const
{
    return strand_.running_in_this_thread();
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

// private
void p2p::do_load(const messages::address_item& host)
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.store(host);
}

// TODO: use pointer.
void p2p::load(const messages::address_items& hosts)
{
    boost::asio::post(strand_, std::bind(&p2p::do_loads, this, hosts));
}

// private
void p2p::do_loads(const messages::address_items& hosts)
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.store(hosts);
}

void p2p::unload(const messages::address_item& host)
{
    boost::asio::post(strand_, std::bind(&p2p::do_unload, this, host));
}

// private
void p2p::do_unload(const messages::address_item& host)
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.remove(host);
}

void p2p::fetch_address(hosts::address_item_handler handler) const
{
    boost::asio::post(
        strand_, std::bind(&p2p::do_fetch_address, this, std::move(handler)));
}

// private
void p2p::do_fetch_address(hosts::address_item_handler handler) const
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.fetch(handler);
}

void p2p::fetch_addresses(hosts::address_items_handler handler) const
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_fetch_addresses, this, std::move(handler)));
}

// private
void p2p::do_fetch_addresses(hosts::address_items_handler handler) const
{
    BC_ASSERT_MSG(stranded(), "hosts_");
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

// private
void p2p::do_pend(uint64_t nonce)
{
    BC_ASSERT_MSG(stranded(), "nonces_");
    nonces_.insert(nonce);
}

void p2p::unpend(uint64_t nonce)
{
    boost::asio::post(strand_, std::bind(&p2p::do_unpend, this, nonce));
}

// private
void p2p::do_unpend(uint64_t nonce)
{
    BC_ASSERT_MSG(stranded(), "nonces_");
    nonces_.erase(nonce);
}

void p2p::store(channel::ptr channel, bool notify, bool inbound,
    result_handler handler)
{
    boost::asio::post(strand_,
        std::bind(&p2p::do_store, this, channel, notify, inbound, handler));
}

// private
// Channel is presumed to be started.
void p2p::do_store(channel::ptr channel, bool notify, bool inbound,
    result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "do_store (multiple members)");

    // Cannot allow any storage once stopped, or do_stop will not free it.
    if (!manual_)
    {
        handler(error::service_stopped);
        return;
    }

    // Check for connection incoming from outgoing self.
    if (inbound &&
        nonces_.find(channel->peer_version()->nonce) != nonces_.end())
    {
        handler(error::accept_failed);
        return;
    }

    // Store the peer address, fail if already exists.
    if (!authorities_.insert(channel->authority()).second)
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

// private
// Channel is presumed to be stopped.
void p2p::do_unstore(channel::ptr channel)
{
    BC_ASSERT_MSG(stranded(), "channels_, authorities_");
    --channel_count_;
    channels_.erase(channel);
    authorities_.erase(channel->authority());
}

// Specializations (protected).
// ----------------------------------------------------------------------------
// Create derived sessions and override these to inject from derived p2p class.

// protected
session_seed::ptr p2p::attach_seed_session()
{
    BC_ASSERT_MSG(stranded(), "attach (do_subscribe_close)");
    return attach<session_seed>();
}

// protected
session_manual::ptr p2p::attach_manual_session()
{
    BC_ASSERT_MSG(stranded(), "attach (do_subscribe_close)");
    return attach<session_manual>();
}

// protected
session_inbound::ptr p2p::attach_inbound_session()
{
    BC_ASSERT_MSG(stranded(), "attach (do_subscribe_close)");
    return attach<session_inbound>();
}

// protected
session_outbound::ptr p2p::attach_outbound_session()
{
    BC_ASSERT_MSG(stranded(), "attach (do_subscribe_close)");
    return attach<session_outbound>();
}

} // namespace network
} // namespace libbitcoin
