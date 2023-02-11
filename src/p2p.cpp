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
#include <cstdlib>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/sessions.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

using namespace bc::system;
using namespace std::placeholders;

p2p::p2p(const settings& settings, const logger& log) NOEXCEPT
  : settings_(settings),
    channel_count_(zero),
    inbound_channel_count_(zero),
    hosts_(settings_),
    threadpool_(settings_.threads),
    strand_(threadpool_.service().get_executor()),
    stop_subscriber_(strand_),
    channel_subscriber_(strand_),
    reporter(log)
{
    BC_ASSERT_MSG(!is_zero(settings.threads), "empty threadpool");
}

p2p::~p2p() NOEXCEPT
{
    // Weak references in threadpool closures safe as p2p joins threads here.
    p2p::close();
}

// I/O factories.
// ----------------------------------------------------------------------------

acceptor::ptr p2p::create_acceptor() NOEXCEPT
{
    return std::make_shared<acceptor>(log(), strand(), service(),
        network_settings());
}

connector::ptr p2p::create_connector() NOEXCEPT
{
    return std::make_shared<connector>(log(), strand(), service(),
        network_settings());
}

connectors_ptr p2p::create_connectors(size_t count) NOEXCEPT
{
    const auto connects = std::make_shared<connectors>();
    connects->reserve(count);

    for (size_t connect = 0; connect < count; ++connect)
        connects->push_back(create_connector());

    return connects;
}

// Start sequence.
// ----------------------------------------------------------------------------

void p2p::start(result_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_start, this, std::move(handler)));
}

void p2p::do_start(const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // manual_ doubles as the closed indicator.
    manual_ = attach_manual_session();
    manual_->start(std::bind(&p2p::handle_start, this, _1, handler));
}

void p2p::handle_start(const code& ec, const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Manual sessions cannot be bypassed.
    if (ec)
    {
        handler(ec);
        return;
    }

    // Host population always required.
    if (const auto error_code = start_hosts())
    {
        LOG("Hosts file failed to deserialize, " << error_code.message());
        handler(error_code);
        return;
    }

    attach_seed_session()->start([handler, this](const code& ec)
    {
        BC_ASSERT_MSG(this->stranded(), "handler");
        handler(ec == error::bypassed ? error::success : ec);
    });
}

// Run sequence (seeding may be ongoing after its handler is invoked).
// ----------------------------------------------------------------------------

void p2p::run(result_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_run, this, std::move(handler)));
}

void p2p::do_run(const result_handler& handler) NOEXCEPT
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
        std::bind(&p2p::handle_run, this, _1, handler));
}

void p2p::handle_run(const code& ec, const result_handler& handler) NOEXCEPT
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

// Not thread safe (threadpool_, hosts_), call only once.
// Results in std::abort if called from a thread within the threadpool.
void p2p::close() NOEXCEPT
{
    boost::asio::dispatch(strand_, std::bind(&p2p::do_close, this));

    // Blocks on join of all threadpool threads.
    if (!threadpool_.join())
    {
        BC_ASSERT_MSG(false, "failed to join threadpool");
        std::abort();
    }

    // Serialize hosts to file.
    if (const auto error_code = stop_hosts())
    {
        LOG("Hosts file failed to serialize, " << error_code.message());
    }
}

void p2p::do_close() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // manual_ is also the closed indicator.
    // Release reference to manual session (also held by stop subscriber).
    if (manual_)
        manual_.reset();

    // Notify and delete all stop subscribers (all sessions).
    stop_subscriber_.stop(error::service_stopped);

    // Notify and delete subscribers to channel notifications.
    channel_subscriber_.stop_default(error::service_stopped);

    // Stop all channels.
    for (const auto& channel: channels_)
        channel->stop(error::service_stopped);

    // Free all channels.
    channels_.clear();

    // Stop threadpool keep-alive, all work must self-terminate to affect join.
    threadpool_.stop();
}

// Subscriptions.
// ----------------------------------------------------------------------------

// public
void p2p::subscribe_connect(channel_handler&& handler,
    result_handler&& complete) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_subscribe_connect,
            this, std::move(handler), std::move(complete)));
}

void p2p::do_subscribe_connect(const channel_handler& handler,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    channel_subscriber_.subscribe(move_copy(handler));
    complete(error::success);
}

// private
void p2p::subscribe_close(result_handler&& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_.subscribe(std::move(handler));
}

// public
void p2p::subscribe_close(result_handler&& handler,
    result_handler&& complete) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_subscribe_close,
            this, std::move(handler), std::move(complete)));
}

void p2p::do_subscribe_close(const result_handler& handler,
    const result_handler& complete) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    stop_subscriber_.subscribe(move_copy(handler));
    complete(error::success);
}

// Manual connections.
// ----------------------------------------------------------------------------

void p2p::connect(const config::endpoint& endpoint) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_connect, this, endpoint));
}

void p2p::do_connect(const config::endpoint& endpoint) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (manual_)
        manual_->connect(endpoint);
}

void p2p::connect(const config::endpoint& endpoint,
    channel_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_connect_handled, this, endpoint,
            std::move(handler)));
}

void p2p::do_connect_handled(const config::endpoint& endpoint,
    const channel_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (manual_)
        manual_->connect(endpoint, move_copy(handler));
    else
        handler(error::service_stopped, nullptr);
}

// Properties.
// ----------------------------------------------------------------------------

// private
bool p2p::closed() const NOEXCEPT
{
    // manual_ doubles as the closed indicator.
    BC_ASSERT_MSG(stranded(), "strand");
    return !manual_;
}

size_t p2p::address_count() const NOEXCEPT
{
    return hosts_.count();
}

size_t p2p::channel_count() const NOEXCEPT
{
    return channel_count_;
}

size_t p2p::inbound_channel_count() const NOEXCEPT
{
    return inbound_channel_count_;
}

const settings& p2p::network_settings() const NOEXCEPT
{
    return settings_;
}

asio::io_context& p2p::service() NOEXCEPT
{
    return threadpool_.service();
}

asio::strand& p2p::strand() NOEXCEPT
{
    return strand_;
}

// protected
bool p2p::stranded() const NOEXCEPT
{
    return strand_.running_in_this_thread();
}

// Hosts collection.
// ----------------------------------------------------------------------------

// private
code p2p::start_hosts() NOEXCEPT
{
    return hosts_.start();
}

// private
code p2p::stop_hosts() NOEXCEPT
{
    return hosts_.stop();
}

void p2p::take(address_item_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_take, this, std::move(handler)));
}

void p2p::do_take(const address_item_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    hosts_.take(handler);
}

void p2p::restore(const address_item_cptr& host,
    result_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_restore, this, host, std::move(handler)));
}

void p2p::do_restore(const address_item_cptr& host,
    const result_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    handler(hosts_.restore(*host) ? error::success : error::address_invalid);
}

void p2p::fetch(address_handler&& handler) const NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_fetch, this, std::move(handler)));
}

void p2p::do_fetch(const address_handler& handler) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    hosts_.fetch(handler);
}

void p2p::save(const address_cptr& message,
    count_handler&& handler) NOEXCEPT
{
    boost::asio::dispatch(strand_,
        std::bind(&p2p::do_save, this, message, std::move(handler)));
}

void p2p::do_save(const messages::address::cptr& message,
    const count_handler& handler) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    handler(error::success, hosts_.save(message->addresses));
}

// Connection management.
// ----------------------------------------------------------------------------

// Preclude pending redundant channel nonces.
bool p2p::pend(uint64_t nonce) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return settings_.enable_loopback || nonces_.insert(nonce).second;
}

bool p2p::unpend(uint64_t nonce) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return settings_.enable_loopback || to_bool(nonces_.erase(nonce));
}

code p2p::store(const channel::ptr& channel, bool notify,
    bool inbound) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Cannot allow any storage once stopped, or do_stop will not free it.
    if (closed())
        return error::service_stopped;

    // Check for incoming connection from outgoing self (loopback).
    if (!settings_.enable_loopback && 
        inbound && to_bool(nonces_.count(channel->peer_version()->nonce)))
        return error::accept_failed;

    // Guard against integer overflow.
    if (inbound && is_zero(add1(inbound_channel_count_.load())))
        return error::channel_overflow;

    // Guard against integer overflow.
    if (is_zero(add1(channel_count_.load())))
        return error::channel_overflow;

    // Store peer address, unless channel/authority is non-distinct.
    if (!authorities_.insert(channel->authority()).second)
        return error::address_in_use;

    // Notify channel subscribers of started channel.
    if (notify)
        channel_subscriber_.notify(error::success, channel);

    // Increment inbound channel counter.
    if (inbound)
        ++inbound_channel_count_;

    // Increment total channel counter.
    ++channel_count_;

    // Store channel for message broadcast and stop notification.
    channels_.insert(channel);
    return error::success;
}

code p2p::unstore(const channel::ptr& channel, bool inbound) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Ok if not found, as the channel may not have been stored.
    if (is_zero(channels_.erase(channel)))
        return error::success;

    // Guard against integer underflow.
    if (inbound && is_zero(inbound_channel_count_.load()))
        return error::channel_underflow;

    // Guard against integer underflow.
    if (is_zero(channel_count_.load()))
        return error::channel_underflow;

    // Decrement inbound channel counter.
    if (inbound)
        --inbound_channel_count_;

    // Decrement total channel counter.
    --channel_count_;

    // Unstore peer address (ok if not found).
    authorities_.erase(channel->authority());
    return  error::success;
}

// Specializations (protected).
// ----------------------------------------------------------------------------

session_seed::ptr p2p::attach_seed_session() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return attach<session_seed>(*this);
}

session_manual::ptr p2p::attach_manual_session() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return attach<session_manual>(*this);
}

session_inbound::ptr p2p::attach_inbound_session() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return attach<session_inbound>(*this);
}

session_outbound::ptr p2p::attach_outbound_session() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return attach<session_outbound>(*this);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
