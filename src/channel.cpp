/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/channel.hpp>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::message;
using namespace std::placeholders;

// Factory for deadline timer pointer construction.
static deadline::ptr alarm(threadpool& pool, const asio::duration& duration)
{
    return std::make_shared<deadline>(pool, pseudo_random::duration(duration));
}

channel::channel(threadpool& pool, socket::ptr socket,
    const settings& settings)
  : proxy(pool, socket, settings),
    notify_(false),
    nonce_(0),
    expiration_(alarm(pool, settings.channel_expiration())),
    inactivity_(alarm(pool, settings.channel_inactivity())),
    CONSTRUCT_TRACK(channel)
{
}

// Talk sequence.
// ----------------------------------------------------------------------------

// public:
void channel::start(result_handler handler)
{
    proxy::start(
        std::bind(&channel::do_start,
            shared_from_base<channel>(), _1, handler));
}

// Don't start the timers until the socket is enabled.
void channel::do_start(const code& , result_handler handler)
{
    start_expiration();
    start_inactivity();
    handler(error::success);
}

// Properties.
// ----------------------------------------------------------------------------

bool channel::notify() const
{
    return notify_;
}

void channel::set_notify(bool value)
{
    notify_ = value;
}

uint64_t channel::nonce() const
{
    return nonce_;
}

void channel::set_nonce(uint64_t value)
{
    nonce_.store(value);
}

version_const_ptr channel::peer_version() const
{
    const auto version = peer_version_.load();
    BITCOIN_ASSERT_MSG(version, "Read peer version before set.");
    return version;
}

void channel::set_peer_version(version_const_ptr value)
{
    peer_version_.store(value);
}

// Proxy pure virtual protected and ordered handlers.
// ----------------------------------------------------------------------------

// It is possible that this may be called multiple times.
void channel::handle_stopping()
{
    expiration_->stop();
    inactivity_->stop();
}

void channel::signal_activity()
{
    start_inactivity();
}

bool channel::stopped(const code& ec) const
{
    return proxy::stopped() || ec == error::channel_stopped ||
        ec == error::service_stopped;
}

// Timers (these are inherent races, requiring stranding by stop only).
// ----------------------------------------------------------------------------

void channel::start_expiration()
{
    if (proxy::stopped())
        return;

    expiration_->start(
        std::bind(&channel::handle_expiration,
            shared_from_base<channel>(), _1));
}

void channel::handle_expiration(const code& ec)
{
    if (stopped(ec))
        return;

    LOG_DEBUG(LOG_NETWORK)
        << "Channel lifetime expired [" << authority() << "]";

    stop(error::channel_timeout);
}

void channel::start_inactivity()
{
    if (proxy::stopped())
        return;

    inactivity_->start(
        std::bind(&channel::handle_inactivity,
            shared_from_base<channel>(), _1));
}

void channel::handle_inactivity(const code& ec)
{
    if (stopped(ec))
        return;

    LOG_DEBUG(LOG_NETWORK)
        << "Channel inactivity timeout [" << authority() << "]";

    stop(error::channel_timeout);
}

} // namespace network
} // namespace libbitcoin
