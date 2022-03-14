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

////// This can be exceeded due to manual connection calls and race conditions.
////inline size_t nominal_connecting(const settings& settings)
////{
////    return settings.peers.size() + settings.connect_batch_size *
////        settings.outbound_connections;
////}
////
////// This can be exceeded due to manual connection calls and race conditions.
////inline size_t nominal_connected(const settings& settings)
////{
////    return settings.peers.size() + settings.outbound_connections +
////        settings.inbound_connections;
////}

p2p::p2p(const settings& settings)
  : settings_(settings),
    channel_count_(zero),
    inbound_channel_count_(zero),
    hosts_(settings_),
    threadpool_(settings_.threads),
    strand_(threadpool_.service().get_executor()),
    stop_subscriber_(std::make_shared<stop_subscriber>(strand_)),
    channel_subscriber_(std::make_shared<channel_subscriber>(strand_))
{
    BC_ASSERT_MSG(!is_zero(settings.threads), "empty threadpool");
}

p2p::~p2p()
{
    p2p::close();
}

// I/O factories.
// ----------------------------------------------------------------------------

acceptor::ptr p2p::create_acceptor()
{
    return std::make_shared<acceptor>(strand(), service(), network_settings());
}

connector::ptr p2p::create_connector()
{
    return std::make_shared<connector>(strand(), service(), network_settings());
}

connectors_ptr p2p::create_connectors(size_t count)
{
    const auto connects = std::make_shared<connectors>(connectors{});
    connects->reserve(count);

    for (size_t connect = 0; connect < count; ++connect)
        connects->push_back(create_connector());

    return connects;
}

// Start sequence.
// ----------------------------------------------------------------------------

void p2p::start(result_handler handler)
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_start, this, std::move(handler)));
}

void p2p::do_start(result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "attach_manual_session");

    manual_ = attach_manual_session();
    manual_->start(std::bind(&p2p::handle_start, this, _1, std::move(handler)));
}

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
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_run, this, std::move(handler)));
}

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
        std::bind(&p2p::handle_run, this, _1, std::move(handler)));
}

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

// Not thread safe.
// Blocks on join of all threadpool threads.
// Must not be called from a thread within threadpool_.
void p2p::close()
{
    boost::asio::dispatch(strand_, std::bind(&p2p::do_close, this));
    threadpool_.join();
}

void p2p::do_close()
{
    BC_ASSERT_MSG(stranded(), "do_stop (multiple members)");

    // Release reference to manual session (also held by stop subscriber).
    if (manual_)
        manual_.reset();

    // Notify and delete all stop subscribers (all sessions).
    stop_subscriber_->stop(error::service_stopped);

    // Notify and delete subscribers to channel notifications.
    channel_subscriber_->stop(error::service_stopped, nullptr);

    // Release all channels.
    channels_.clear();

    // Serialize hosts file (log results).
    /*code*/ hosts_.stop();

    // Stop threadpool keep-alive, all work must self-terminate to affect join.
    threadpool_.stop();
}

// Subscriptions.
// ----------------------------------------------------------------------------

// External or derived callers.
void p2p::subscribe_connect(channel_handler handler, result_handler complete)
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_subscribe_connect,
            this, std::move(handler), std::move(complete)));
}

void p2p::do_subscribe_connect(channel_handler handler, result_handler complete)
{
    BC_ASSERT_MSG(stranded(), "channel_subscriber_");
    channel_subscriber_->subscribe(std::move(handler));
    complete(error::success);
}

void p2p::subscribe_close(result_handler handler, result_handler complete)
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_subscribe_close2,
            this, std::move(handler), std::move(complete)));
}

void p2p::do_subscribe_close(result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "stop_subscriber_");
    stop_subscriber_->subscribe(std::move(handler));
}

void p2p::do_subscribe_close2(result_handler handler, result_handler complete)
{
    BC_ASSERT_MSG(stranded(), "stop_subscriber_");
    stop_subscriber_->subscribe(std::move(handler));
    complete(error::success);
}

// Manual connections.
// ----------------------------------------------------------------------------

void p2p::connect(const config::endpoint& endpoint)
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_connect1, this, endpoint));
}

void p2p::connect(const std::string& hostname, uint16_t port)
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_connect2, this, hostname, port));
}

void p2p::connect(const std::string& hostname, uint16_t port,
    channel_handler handler)
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_connect3, this, hostname, port, std::move(handler)));
}

void p2p::do_connect1(const config::endpoint& endpoint)
{
    BC_ASSERT_MSG(stranded(), "manual_");

    if (manual_)
        manual_->connect(endpoint.host(), endpoint.port());
}

void p2p::do_connect2(const std::string& hostname, uint16_t port)
{
    BC_ASSERT_MSG(stranded(), "manual_");

    if (manual_)
        manual_->connect(hostname, port);
}

void p2p::do_connect3(const std::string& hostname, uint16_t port,
    channel_handler handler)
{
    BC_ASSERT_MSG(stranded(), "manual_");

    if (manual_)
        manual_->connect(hostname, port, handler);
    else
        handler(error::service_stopped, nullptr);
}

// Properties.
// ----------------------------------------------------------------------------

size_t p2p::address_count() const
{
    return hosts_.count();
}

size_t p2p::channel_count() const
{
    return channel_count_.load(std::memory_order_relaxed);
}

size_t p2p::inbound_channel_count() const
{
    return inbound_channel_count_.load(std::memory_order_relaxed);
}

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

// protected
bool p2p::stranded() const
{
    return strand_.running_in_this_thread();
}

// Hosts collection.
// ----------------------------------------------------------------------------

void p2p::fetch(hosts::address_item_handler handler) const
{
    boost::asio::dispatch(
        strand_, std::bind(&p2p::do_fetch, this, std::move(handler)));
}

void p2p::do_fetch(hosts::address_item_handler handler) const
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.fetch(handler);
}

void p2p::fetches(hosts::address_items_handler handler) const
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_fetches, this, std::move(handler)));
}

void p2p::do_fetches(hosts::address_items_handler handler) const
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.fetch(handler);
}

void p2p::dump(const messages::address_item& host, result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.remove(host);
    handler(error::success);
}

void p2p::save(const messages::address_item& host, result_handler handler)
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_save, this, host, std::move(handler)));
}

void p2p::do_save(const messages::address_item& host, result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.store(host);
    handler(error::success);
}

// TODO: use pointer.
void p2p::saves(const messages::address_items& hosts, result_handler handler)
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_saves, this, hosts, std::move(handler)));
}

void p2p::do_saves(const messages::address_items& hosts, result_handler handler)
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.store(hosts);
    handler(error::success);
}

// Connection management.
// ----------------------------------------------------------------------------

// TODO: if a channel is created with a conflicting nonce, the first deletion
// will remove both, resulting in removal of self-connect protection for first.
void p2p::pend(uint64_t nonce)
{
    BC_ASSERT_MSG(stranded(), "nonces_");
    nonces_.insert(nonce);
}

void p2p::unpend(uint64_t nonce)
{
    BC_ASSERT_MSG(stranded(), "nonces_");
    nonces_.erase(nonce);
}

code p2p::store(channel::ptr channel, bool notify, bool inbound)
{
    BC_ASSERT_MSG(stranded(), "do_store (multiple members)");

    // Cannot allow any storage once stopped, or do_stop will not free it.
    if (!manual_)
        return error::service_stopped;

    // Check for connection incoming from outgoing self.
    if (inbound &&
        nonces_.find(channel->peer_version()->nonce) != nonces_.end())
        return error::accept_failed;

    // Store the peer address, fail if already exists.
    if (!authorities_.insert(channel->authority()).second)
        return error::address_in_use;

    // Notify channel subscribers of started channel.
    if (notify)
        channel_subscriber_->notify(error::success, channel);

    // TODO: guard overflow.
    if (inbound)
        ++inbound_channel_count_;

    // TODO: guard overflow.
    ++channel_count_;
    channels_.insert(channel);
    return error::success;
}

void p2p::unstore(channel::ptr channel, bool inbound)
{
    BC_ASSERT_MSG(stranded(), "channels_, authorities_");

    // TODO: guard underflow.
    if (inbound)
        --inbound_channel_count_;

    // TODO: guard underflow.
    --channel_count_;
    channels_.erase(channel);
    authorities_.erase(channel->authority());
}

// Specializations (protected).
// ----------------------------------------------------------------------------

// protected
session_seed::ptr p2p::attach_seed_session()
{
    BC_ASSERT_MSG(stranded(), "attach (do_subscribe_close)");
    return do_attach<session_seed>();
}

// protected
session_manual::ptr p2p::attach_manual_session()
{
    BC_ASSERT_MSG(stranded(), "attach (do_subscribe_close)");
    return do_attach<session_manual>();
}

// protected
session_inbound::ptr p2p::attach_inbound_session()
{
    BC_ASSERT_MSG(stranded(), "attach (do_subscribe_close)");
    return do_attach<session_inbound>();
}

// protected
session_outbound::ptr p2p::attach_outbound_session()
{
    BC_ASSERT_MSG(stranded(), "attach (do_subscribe_close)");
    return do_attach<session_outbound>();
}

} // namespace network
} // namespace libbitcoin
