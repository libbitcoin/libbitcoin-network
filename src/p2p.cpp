/**
 * Copyright (c) 2011-2016 libbitcoin developers (see AUTHORS)
 *
 * This file is part of libbitcoin.
 *
 * libbitcoin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License with
 * additional permissions to the one published by the Free Software
 * Foundation, either version 3 of the License, or (at your option)
 * any later version. For more information see LICENSE.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bitcoin/network/p2p.hpp>

#include <cstddef>
#include <cstdint>
#include <future>
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/connections.hpp>
#include <bitcoin/network/hosts.hpp>
#include <bitcoin/network/protocols/protocol_address.hpp>
#include <bitcoin/network/protocols/protocol_ping.hpp>
#include <bitcoin/network/protocols/protocol_seed.hpp>
#include <bitcoin/network/protocols/protocol_version.hpp>
#include <bitcoin/network/sessions/session_inbound.hpp>
#include <bitcoin/network/sessions/session_manual.hpp>
#include <bitcoin/network/sessions/session_outbound.hpp>
#include <bitcoin/network/sessions/session_seed.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define NAME "p2p"

using std::placeholders::_1;

p2p::p2p(const settings& settings)
  : stopped_(true),
    height_(0),
    settings_(settings),
    hosts_(std::make_shared<hosts>(threadpool_, settings_)),
    connections_(std::make_shared<connections>()),
    stop_subscriber_(std::make_shared<stop_subscriber>(threadpool_, NAME "_stop_sub")),
    channel_subscriber_(std::make_shared<channel_subscriber>(threadpool_, NAME "_sub"))
{
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

    threadpool_.join();
    threadpool_.spawn(settings_.threads, thread_priority::low);

    stopped_ = false;
    stop_subscriber_->start();
    channel_subscriber_->start();

    // This instance is retained by stop handler and member references.
    const auto manual = attach<session_manual>();
    manual_.store(manual);

    // This is invoked on a new thread.
    manual->start(
        std::bind(&p2p::handle_manual_started,
            this, _1, handler));
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
        log::error(LOG_NETWORK)
            << "Error starting manual session: " << ec.message();
        handler(ec);
        return;
    }

    handle_hosts_loaded(hosts_->load(), handler);
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
        log::error(LOG_NETWORK)
            << "Error loading host addresses: " << ec.message();
        handler(ec);
        return;
    }

    // This is invoked on a new thread.
    // The instance is retained by the stop handler (until shutdown).
    attach<session_seed>()->start(
        std::bind(&p2p::handle_started,
            this, _1, handler));
}

void p2p::handle_started(const code& ec, result_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped);
        return;
    }

    if (ec)
    {
        log::error(LOG_NETWORK)
            << "Error seeding host addresses: " << ec.message();
        handler(ec);
        return;
    }

    // There is no way to guarantee subscription before handler execution.
    // So currently subscription for seed node connections is not supported.
    // Subscription after this return will capture connections established via
    // subsequent "run" and "connect" calls, and will clear on close/destruct.

    // This is the end of the start sequence.
    handler(error::success);
}

// Run sequence.
// ----------------------------------------------------------------------------

void p2p::run(result_handler handler)
{
    // This is invoked on a new thread.
    // This instance is retained by the stop handler (until shutdown).
    attach<session_inbound>()->start(
        std::bind(&p2p::handle_inbound_started,
            this, _1, handler));
}

void p2p::handle_inbound_started(const code& ec, result_handler handler)
{
    if (ec)
    {
        log::error(LOG_NETWORK)
            << "Error starting inbound session: " << ec.message();
        handler(ec);
        return;
    }

    // This is invoked on a new thread.
    // This instance is retained by the stop handler (until shutdown).
    attach<session_outbound>()->start(
        std::bind(&p2p::handle_running,
            this, _1, handler));
}

void p2p::handle_running(const code& ec, result_handler handler)
{
    if (ec)
    {
        log::error(LOG_NETWORK)
            << "Error starting outbound session: " << ec.message();
        handler(ec);
        return;
    }

    // This is the end of the run sequence.
    handler(error::success);
}

// Stop sequence.
// ----------------------------------------------------------------------------
// All shutdown actions must be queued by the end of the stop call.
// IOW queued shutdown operations must not enqueue additional work.

// This is not short-circuited by a stop test because we need to ensure it
// completes at least once before invoking the handler. This requires a unique
// lock be taken around the entire section, which poses a deadlock risk.
// Instead this is thread safe and idempotent, allowing it to be unguarded.
void p2p::stop(result_handler handler)
{
    // Host save is expensive, so minimize repeats.
    const auto ec = stopped_ ? error::success : hosts_->save();

    if (ec)
        log::error(LOG_NETWORK)
            << "Error saving hosts file: " << ec.message();

    stopped_ = true;

    // Prevent subscription after stop.
    stop_subscriber_->stop();
    stop_subscriber_->do_relay(error::service_stopped);

    // Prevent subscription after stop.
    channel_subscriber_->stop();
    channel_subscriber_->do_relay(error::service_stopped, nullptr);

    // Stop accepting channels and stop those that exist (self-clearing).
    connections_->stop(error::service_stopped);

    manual_.store(nullptr);
    threadpool_.shutdown();

    // This indirection is not required but presented for consistency.
    handle_stopped(ec, handler);
}

void p2p::handle_stopped(const code& ec, result_handler handler)
{
    // This is the end of the stop sequence.
    handler(ec);
}

// Close sequence.
// ----------------------------------------------------------------------------

// This allows for shutdown based on destruct without need to call stop.
p2p::~p2p()
{
    p2p::close();
}

// This must be called from the thread that constructed this class (see join).
void p2p::close()
{
    std::promise<code> wait;

    p2p::stop(
        std::bind(&p2p::handle_closing,
            this, _1, std::ref(wait)));

    // This blocks until handle_closing completes.
    wait.get_future();
}

void p2p::handle_closing(const code& ec, std::promise<code>& wait)
{
    threadpool_.join();

    // This is the end of the derived close sequence.
    wait.set_value(ec);
}

// Properties.
// ----------------------------------------------------------------------------

const settings& p2p::network_settings() const
{
    return settings_;
}

// The blockchain height is set in our version message for handshake.
size_t p2p::height() const
{
    return height_;
}

// The height is set externally and is safe as an atomic.
void p2p::set_height(size_t value)
{
    height_ = value;
}

bool p2p::stopped() const
{
    return stopped_;
}

threadpool& p2p::thread_pool()
{
    return threadpool_;
}

// Subscriptions.
// ----------------------------------------------------------------------------

void p2p::subscribe_connection(connect_handler handler)
{
    channel_subscriber_->subscribe(handler, error::service_stopped, nullptr);
}

void p2p::subscribe_stop(result_handler handler)
{
    stop_subscriber_->subscribe(handler, error::service_stopped);
}

// Manual connections.
// ----------------------------------------------------------------------------

void p2p::connect(const std::string& hostname, uint16_t port)
{
    if (stopped())
        return;

    auto manual = manual_.load();
    if (manual)
        manual->connect(hostname, port);
}

void p2p::connect(const std::string& hostname, uint16_t port,
    channel_handler handler)
{
    if (stopped())
    {
        handler(error::service_stopped, nullptr);
        return;
    }

    auto manual = manual_.load();
    if (manual)
    {
        // Connect is invoked on a new thread.
        manual->connect(hostname, port, handler);
    }
}

// Connections collection.
// ----------------------------------------------------------------------------

void p2p::connected(const address& address, truth_handler handler)
{
    connections_->exists(address, handler);
}

void p2p::store(channel::ptr channel, result_handler handler)
{
    const auto new_connection_handler =
        std::bind(&p2p::handle_new_connection,
            this, _1, channel, handler);

    connections_->store(channel, new_connection_handler);
}

void p2p::handle_new_connection(const code& ec, channel::ptr channel,
    result_handler handler)
{
    // Connection-in-use indicated here by error::address_in_use.
    handler(ec);
    
    if (!ec && channel->notify())
        channel_subscriber_->relay(error::success, channel);
}

void p2p::remove(channel::ptr channel, result_handler handler)
{
    connections_->remove(channel, handler);
}

void p2p::connected_count(count_handler handler)
{
    connections_->count(handler);
}

// Hosts collection.
// ----------------------------------------------------------------------------

void p2p::fetch_address(address_handler handler)
{
    address out;
    handler(hosts_->fetch(out), out);
}

void p2p::store(const address& address, result_handler handler)
{
    handler(hosts_->store(address));
}

void p2p::store(const address::list& addresses, result_handler handler)
{
    // Store is invoked on a new thread.
    hosts_->store(addresses, handler);
}

void p2p::remove(const address& address, result_handler handler)
{
    handler(hosts_->remove(address));
}

void p2p::address_count(count_handler handler)
{
    handler(hosts_->count());
}

} // namespace network
} // namespace libbitcoin
