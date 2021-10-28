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
#include <bitcoin/network/net/channel.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace bc::system;
using namespace bc::system::messages;
using namespace std::placeholders;

// Helper to derive maximum message payload size from settings.
inline size_t payload_maximum(const settings& settings)
{
    return heading::maximum_payload_size(settings.protocol_maximum,
        !is_zero(settings.services & version::service::node_witness));
}

// Factory for fixed deadline timer pointer construction.
inline deadline::ptr timeout(asio::strand& strand, const duration& span)
{
    return std::make_shared<deadline>(strand, span);
}

// Factory for varied deadline timer pointer construction.
inline deadline::ptr expiration(asio::strand& strand, const duration& span)
{
    return timeout(strand, pseudo_random::duration(span));
}

channel::channel(socket::ptr socket, const settings& settings)
  : proxy(socket),
    maximum_payload_(payload_maximum(settings)),
    protocol_magic_(settings.identifier),
    validate_checksum_(settings.validate_checksum),
    verbose_logging_(settings.verbose),
    notify_on_connect_(false),
    channel_nonce_(0),
    negotiated_version_(settings.protocol_maximum),
    peer_version_(std::make_shared<system::messages::version>()),
    expiration_(expiration(socket->strand(), settings.channel_expiration())),
    inactivity_(timeout(socket->strand(), settings.channel_inactivity())),
    CONSTRUCT_TRACK(channel)
{
}

// Start/stop.
// ----------------------------------------------------------------------------

void channel::start()
{
    start_expiration();
    start_inactivity();
    proxy::start();
}

void channel::stop(const code& ec)
{
    inactivity_->stop();
    expiration_->stop();
    proxy::stop(ec);
}

// Properties.
// ----------------------------------------------------------------------------

bool channel::notify() const
{
    return notify_on_connect_;
}

void channel::set_notify(bool value)
{
    notify_on_connect_ = value;
}

uint64_t channel::nonce() const
{
    return channel_nonce_;
}

// TODO: can the nonce be generated internally on construct?
void channel::set_nonce(uint64_t value)
{
    channel_nonce_ = value;
}

uint32_t channel::negotiated_version() const
{
    return negotiated_version_;
}

void channel::set_negotiated_version(uint32_t value)
{
    negotiated_version_ = value;
}

version_const_ptr channel::peer_version() const
{
    return peer_version_.load();
}

void channel::set_peer_version(version_const_ptr value)
{
    peer_version_.store(value);
}

// Proxy overrides (channel maintains state for the proxy).
// ----------------------------------------------------------------------------
// private

size_t channel::maximum_payload() const
{
    return maximum_payload_;
}

uint32_t channel::protocol_magic() const
{
    return protocol_magic_;
}

bool channel::validate_checksum() const
{
    return validate_checksum_;
}

bool channel::verbose() const
{
    return verbose_logging_;
}

uint32_t channel::version() const
{
    return negotiated_version();
}

// Cancels previous timer and retains configured duration.
// A canceled timer does not invoke its completion handler.
void channel::signal_activity()
{
    return start_inactivity();
}

// Timers.
// ----------------------------------------------------------------------------
// private

void channel::start_expiration()
{
    if (stopped())
        return;

    // Handler is posted to the socket strand.
    expiration_->start(
        std::bind(&channel::handle_expiration,
            shared_from_base<channel>(), _1));
}

void channel::handle_expiration(const code& ec)
{
    BITCOIN_ASSERT_MSG(strand().running_in_this_thread(), "strand");

    if (stopped())
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Channel lifetime timer failure [" << authority() << "] "
            << ec.message();
        stop(ec);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Channel lifetime expired [" << authority() << "]";
    stop(ec);
}

void channel::start_inactivity()
{
    if (stopped())
        return;

    // Handler is posted to the socket strand.
    inactivity_->start(
        std::bind(&channel::handle_inactivity,
            shared_from_base<channel>(), _1));
}

void channel::handle_inactivity(const code& ec)
{
    BITCOIN_ASSERT_MSG(strand().running_in_this_thread(), "strand");

    if (stopped())
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Channel inactivity timer failure [" << authority() << "] "
            << ec.message();
        stop(ec);
        return;
    }

    LOG_DEBUG(LOG_NETWORK)
        << "Channel inactivity timeout [" << authority() << "]";
    stop(ec);
}

} // namespace network
} // namespace libbitcoin
