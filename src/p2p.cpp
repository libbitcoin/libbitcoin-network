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
using namespace boost::asio;

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
    p2p::do_stop(result_handler{});
}

// Start sequence.
// ----------------------------------------------------------------------------

void p2p::start(result_handler handler)
{
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

// Threads are joined when handler is invoked.
void p2p::stop(result_handler handler)
{
    stopped_.store(true, std::memory_order_relaxed);
    post(strand_, std::bind(&p2p::do_stop, this, std::move(handler)));
}

void p2p::do_stop(result_handler handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");

    stop_subscriber_->stop(error::service_stopped);
    channel_subscriber_->stop(error::service_stopped, nullptr);

    threadpool_.stop();
    threadpool_.join();
    handler(hosts_.stop());
}

bool p2p::stopped() const
{
    return stopped_.load(std::memory_order_relaxed);
}

// Subscriptions.
// ----------------------------------------------------------------------------

void p2p::subscribe_connection(connect_handler handler)
{
    post(strand_, std::bind(&p2p::do_subscribe_connection, this, std::move(handler)));
}

void p2p::do_subscribe_connection(connect_handler handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    channel_subscriber_->subscribe(handler);
}

void p2p::subscribe_stop(result_handler handler)
{
    post(strand_, std::bind(&p2p::do_subscribe_stop, this, std::move(handler)));
}

void p2p::do_subscribe_stop(result_handler handler)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    stop_subscriber_->subscribe(handler);
}

// Manual connections.
// ----------------------------------------------------------------------------

void p2p::connect(const config::endpoint& peer)
{
    manual_->connect(peer.host(), peer.port());
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
    return attach<session_manual>(true);
}

session_inbound::ptr p2p::attach_inbound_session()
{
    return attach<session_inbound>(true);
}

session_outbound::ptr p2p::attach_outbound_session()
{
    return attach<session_outbound>(true);
}

// Hosts collection.
// ----------------------------------------------------------------------------

size_t p2p::address_count() const
{
    return hosts_.count();
}

void p2p::store(const messages::address_item& host)
{
    post(strand_, std::bind(&p2p::do_store_host, this, host));
}

void p2p::do_store_host(const messages::address_item& host)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.store(host);
}

// TODO: use pointer.
void p2p::store(const messages::address_items& hosts)
{
    post(strand_, std::bind(&p2p::do_store_hosts, this, hosts));
}

void p2p::do_store_hosts(const messages::address_items& hosts)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.store(hosts);
}

void p2p::remove(const messages::address_item& host)
{
    post(strand_, std::bind(&p2p::do_remove, this, host));
}

void p2p::do_remove(const messages::address_item& host)
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.remove(host);
}

void p2p::fetch_address(hosts::peer_handler handler) const
{
    post(strand_, std::bind(&p2p::do_fetch_address, this, std::move(handler)));
}

void p2p::do_fetch_address(hosts::peer_handler handler) const
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.fetch(handler);
}

void p2p::fetch_addresses(hosts::peers_handler handler) const
{
    post(strand_, std::bind(&p2p::do_fetch_addresses, this, std::move(handler)));
}

void p2p::do_fetch_addresses(hosts::peers_handler handler) const
{
    BC_ASSERT_MSG(strand_.running_in_this_thread(), "strand");
    hosts_.fetch(handler);
}

// Shouldn't need to pend connector.

////// Pending connect collection.
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

// These are for detecting reflection.

////// Pending handshake collection.
////// ----------------------------------------------------------------------------
////// TODO: use common collection, nonces are sufficiently unique?
////// TODO: would need to subscribe to broadcast after handshake.
////
////code p2p::pend(channel::ptr channel)
////{
////    return pending_handshake_.store(channel);
////}
////
////void p2p::unpend(channel::ptr channel)
////{
////    pending_handshake_.remove(channel);
////}
////
////bool p2p::pending(uint64_t version_nonce) const
////{
////    const auto match = [version_nonce](const channel::ptr& element)
////    {
////        return element->nonce() == version_nonce;
////    };
////
////    return pending_handshake_.exists(match);
////}

// This can be replaced with broadcast subscription.

// Pending close collection (open connections).
// ----------------------------------------------------------------------------

size_t p2p::connection_count() const
{
    return channels_.size();
}

bool p2p::connected(const messages::address_item& address) const
{
    ////const auto match = [&address](const channel::ptr& element)
    ////{
    ////    return element->authority() == address;
    ////};

    ////return pending_close_.exists(match);
    return false;
}

code p2p::store(channel::ptr channel)
{
    ////const auto address = channel->authority();
    ////const auto match = [&address](const channel::ptr& element)
    ////{
    ////    return element->authority() == address;
    ////};

    ////// May return error::address_in_use.
    ////const auto ec = pending_close_.store(channel, match);

    ////if (!ec && channel->notify())
    ////    channel_subscriber_.notify(error::success, channel);

    ////return ec;
    return {};
}

void p2p::remove(channel::ptr channel)
{
    ////pending_close_.remove(channel);
}

} // namespace network
} // namespace libbitcoin
