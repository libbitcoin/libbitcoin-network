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
#include <cstdlib>
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

p2p::p2p(const settings& settings) noexcept
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

p2p::~p2p() noexcept
{
    // Weak references in threadpool closures safe as p2p joins threads here.
    p2p::close();
}

// I/O factories.
// ----------------------------------------------------------------------------

acceptor::ptr p2p::create_acceptor() noexcept
{
    return std::make_shared<acceptor>(strand(), service(), network_settings());
}

connector::ptr p2p::create_connector() noexcept
{
    return std::make_shared<connector>(strand(), service(), network_settings());
}

connectors_ptr p2p::create_connectors(size_t count) noexcept
{
    const auto connects = std::make_shared<connectors>(connectors{});
    connects->reserve(count);

    for (size_t connect = 0; connect < count; ++connect)
        connects->push_back(create_connector());

    return connects;
}

// Start sequence.
// ----------------------------------------------------------------------------

void p2p::start(result_handler&& handler) noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_start, this, std::move(handler)));
}

void p2p::do_start(const result_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "attach_manual_session");

    // manual_ doubles as the closed indicator.
    manual_ = attach_manual_session();
    manual_->start(std::bind(&p2p::handle_start, this, _1, handler));
}

void p2p::handle_start(const code& ec, const result_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "hosts_");

    // Manual sessions cannot be bypassed.
    if (ec)
    {
        handler(ec);
        return;
    }

    // Host population always required.
    const auto error_code = start_hosts();

    if (error_code)
    {
        handler(error_code);
        return;
    }

    attach_seed_session()->start([handler](const code& ec)
    {
        ////BC_ASSERT_MSG(this->stranded(), "handler");
        handler(ec == error::bypassed ? error::success : ec);
    });
}

// Run sequence (seeding may be ongoing after its handler is invoked).
// ----------------------------------------------------------------------------

void p2p::run(result_handler&& handler) noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_run, this, std::move(handler)));
}

void p2p::do_run(const result_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "manual_, attach_inbound_session");

    if (closed())
    {
        handler(error::service_stopped);
        return;
    }

    for (const auto& peer: settings_.peers)
        do_connect(peer);

    attach_inbound_session()->start(
        std::bind(&p2p::handle_run, this, _1, handler));
}

void p2p::handle_run(const code& ec, const result_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "strand");

    // A bypass code allows continuation.
    if (ec && ec != error::bypassed)
    {
        handler(ec);
        return;
    }

    attach_outbound_session()->start([handler](const code& ec)
    {
        ////BC_ASSERT_MSG(this->stranded(), "handler");
        handler(ec == error::bypassed ? error::success : ec);
    });
}

// Shutdown sequence.
// ----------------------------------------------------------------------------

// Not thread safe (threadpool_), call only once.
// Blocks on join of all threadpool threads.
// Results in std::abort if called from a thread within the threadpool.
void p2p::close() noexcept
{
    boost::asio::dispatch(strand_, std::bind(&p2p::do_close, this));

    if (!threadpool_.join())
    {
        BC_ASSERT_MSG(false, "failed to join threadpool");
        std::abort();
    }
}

void p2p::do_close() noexcept
{
    BC_ASSERT_MSG(stranded(), "do_stop (multiple members)");

    // manual_ doubles as the closed indicator.
    // Release reference to manual session (also held by stop subscriber).
    if (manual_)
        manual_.reset();

    // Notify and delete all stop subscribers (all sessions).
    stop_subscriber_->stop(error::service_stopped);

    // Notify and delete subscribers to channel notifications.
    channel_subscriber_->stop_default(error::service_stopped);

    // Stop all channels.
    for (const auto& channel: channels_)
        channel->stop(error::service_stopped);

    // Free all channels.
    channels_.clear();

    // Serialize hosts file (log results).
    stop_hosts();

    // Stop threadpool keep-alive, all work must self-terminate to affect join.
    threadpool_.stop();
}

// Subscriptions.
// ----------------------------------------------------------------------------

// public
void p2p::subscribe_connect(channel_handler&& handler,
    result_handler&& complete) noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_subscribe_connect,
            this, std::move(handler), std::move(complete)));
}

void p2p::do_subscribe_connect(const channel_handler& handler,
    const result_handler& complete) noexcept
{
    BC_ASSERT_MSG(stranded(), "channel_subscriber_");
    channel_subscriber_->subscribe(move_copy(handler));
    complete(error::success);
}

// private
void p2p::subscribe_close(result_handler&& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "stop_subscriber_");
    stop_subscriber_->subscribe(std::move(handler));
}

// public
void p2p::subscribe_close(result_handler&& handler,
    result_handler&& complete) noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_subscribe_close,
            this, std::move(handler), std::move(complete)));
}

void p2p::do_subscribe_close(const result_handler& handler,
    const result_handler& complete) noexcept
{
    BC_ASSERT_MSG(stranded(), "stop_subscriber_");
    stop_subscriber_->subscribe(move_copy(handler));
    complete(error::success);
}

// Manual connections.
// ----------------------------------------------------------------------------

void p2p::connect(const config::endpoint& endpoint) noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_connect, this, endpoint));
}

void p2p::do_connect(const config::endpoint& endpoint) noexcept
{
    BC_ASSERT_MSG(stranded(), "manual_");

    if (manual_)
        manual_->connect(endpoint);
}

void p2p::connect(const config::endpoint& endpoint,
    channel_handler&& handler) noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_connect_handled, this, endpoint,
            std::move(handler)));
}

void p2p::do_connect_handled(const config::endpoint& endpoint,
    const channel_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "manual_");

    if (manual_)
        manual_->connect(endpoint, move_copy(handler));
    else
        handler(error::service_stopped, nullptr);
}

// Properties.
// ----------------------------------------------------------------------------

// private
bool p2p::closed() const noexcept
{
    BC_ASSERT_MSG(stranded(), "manual_");

    // manual_ doubles as the closed indicator.
    return !manual_;
}

size_t p2p::address_count() const noexcept
{
    return hosts_.count();
}

size_t p2p::channel_count() const noexcept
{
    return channel_count_.load(std::memory_order_relaxed);
}

size_t p2p::inbound_channel_count() const noexcept
{
    return inbound_channel_count_.load(std::memory_order_relaxed);
}

const settings& p2p::network_settings() const noexcept
{
    return settings_;
}

asio::io_context& p2p::service() noexcept
{
    return threadpool_.service();
}

asio::strand& p2p::strand() noexcept
{
    return strand_;
}

// protected
bool p2p::stranded() const noexcept
{
    return strand_.running_in_this_thread();
}

// Hosts collection.
// ----------------------------------------------------------------------------

// private
code p2p::start_hosts() noexcept
{
    return hosts_.start();
}

// private
void p2p::stop_hosts() noexcept
{
    // TODO: log discarded code.
    /* code */ hosts_.stop();
}

void p2p::fetch(hosts::address_item_handler&& handler) const noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_fetch, this, std::move(handler)));
}

void p2p::do_fetch(const hosts::address_item_handler& handler) const noexcept
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.fetch(handler);
}

void p2p::fetches(hosts::address_items_handler&& handler) const noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_fetches, this, std::move(handler)));
}

void p2p::do_fetches(
    const hosts::address_items_handler& handler) const noexcept
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.fetch(handler);
}

void p2p::dump(const messages::address_item& host,
    result_handler&& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.remove(host);
    handler(error::success);
}

void p2p::save(const messages::address_item& host,
    result_handler&& handler) noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_save, this, host, std::move(handler)));
}

void p2p::do_save(const messages::address_item& host,
    const result_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.store(host);
    handler(error::success);
}

// TODO: use pointer.
void p2p::saves(const messages::address_items& hosts,
    result_handler&& handler) noexcept
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_saves, this, hosts, std::move(handler)));
}

void p2p::do_saves(const messages::address_items& hosts,
    const result_handler& handler) noexcept
{
    BC_ASSERT_MSG(stranded(), "hosts_");
    hosts_.store(hosts);
    handler(error::success);
}

// Connection management.
// ----------------------------------------------------------------------------

// TODO: if a channel is created with a conflicting nonce, the first deletion
// will remove both, resulting in removal of self-connect protection for first.
void p2p::pend(uint64_t nonce) noexcept
{
    BC_ASSERT_MSG(stranded(), "nonces_");
    nonces_.insert(nonce);
}

void p2p::unpend(uint64_t nonce) noexcept
{
    BC_ASSERT_MSG(stranded(), "nonces_");
    nonces_.erase(nonce);
}

code p2p::store(const channel::ptr& channel, bool notify,
    bool inbound) noexcept
{
    BC_ASSERT_MSG(stranded(), "do_store (multiple members)");

    // Cannot allow any storage once stopped, or do_stop will not free it.
    if (closed())
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

    if (inbound)
    {
        ++inbound_channel_count_;
        BC_ASSERT_MSG(!is_zero(inbound_channel_count_.load()), "overflow");
    }

    ++channel_count_;
    BC_ASSERT_MSG(!is_zero(channel_count_.load()), "overflow");

    channels_.insert(channel);
    return error::success;
}

bool p2p::unstore(const channel::ptr& channel, bool inbound) noexcept
{
    BC_ASSERT_MSG(stranded(), "channels_, authorities_");

    // Erasure must be idempotent, as the channel may not have been stored.
    if (!is_zero(channels_.erase(channel)))
    {
        if (inbound)
        {
            BC_ASSERT_MSG(!is_zero(inbound_channel_count_.load()), "underflow");
            --inbound_channel_count_;
        }

        BC_ASSERT_MSG(!is_zero(channel_count_.load()), "underflow");
        --channel_count_;

        authorities_.erase(channel->authority());
        return true;
    }

    return false;
}

// Specializations (protected).
// ----------------------------------------------------------------------------

session_seed::ptr p2p::attach_seed_session() noexcept
{
    BC_ASSERT_MSG(stranded(), "attach (subscribe_close)");
    return attach<session_seed>();
}

session_manual::ptr p2p::attach_manual_session() noexcept
{
    BC_ASSERT_MSG(stranded(), "attach (subscribe_close)");
    return attach<session_manual>();
}

session_inbound::ptr p2p::attach_inbound_session() noexcept
{
    BC_ASSERT_MSG(stranded(), "attach (subscribe_close)");
    return attach<session_inbound>();
}

session_outbound::ptr p2p::attach_outbound_session() noexcept
{
    BC_ASSERT_MSG(stranded(), "attach (subscribe_close)");
    return attach<session_outbound>();
}

} // namespace network
} // namespace libbitcoin
