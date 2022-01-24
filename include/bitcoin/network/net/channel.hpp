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
#ifndef LIBBITCOIN_NETWORK_NET_CHANNEL_HPP
#define LIBBITCOIN_NETWORK_NET_CHANNEL_HPP

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/proxy.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

/// A channel is a proxy with logged timers and state.
/// This class is thread safe, though start my be called only once.
/// Stop is thread safe and idempotent, may be called multiple times.
class BCT_API channel
  : public proxy, track<channel>
{
public:
    typedef std::shared_ptr<channel> ptr;

    // Construct.
    // ------------------------------------------------------------------------

    /// Construct an instance from a socket and network settings.
    channel(socket::ptr socket, const settings& settings);

    // Start/stop.
    // ------------------------------------------------------------------------

    /// Start communicating (call only once).
    void start() override;

    /// Cancel work and close the socket (idempotent).
    /// This action is deferred to the strand, not immediately affected.
    /// Block on threadpool.join() to ensure termination of the connection.
    /// Code is passed to stop subscribers, channel_stopped to message pump. 
    void stop(const code& ec) override;

    // Properties.
    // ------------------------------------------------------------------------
    // These are not thread safe, caller must be stranded.

    virtual bool notify() const;
    virtual void set_notify(bool value);

    virtual uint64_t nonce() const;
    virtual void set_nonce(uint64_t value);

    virtual uint32_t negotiated_version() const;
    virtual void set_negotiated_version(uint32_t value);

    virtual messages::version::ptr peer_version() const;
    virtual void set_peer_version(messages::version::ptr value);

private:
    virtual size_t maximum_payload() const override;
    virtual uint32_t protocol_magic() const override;
    virtual bool validate_checksum() const override;
    virtual bool verbose() const override;
    virtual uint32_t version() const override;
    virtual void signal_activity() override;

    virtual void start_expiration();
    void handle_expiration(const code& ec);

    virtual void start_inactivity();
    void handle_inactivity(const code& ec);

    // These are thread safe.
    const size_t maximum_payload_;
    const uint32_t protocol_magic_;
    const bool validate_checksum_;
    const bool verbose_logging_;
    deadline::ptr expiration_;
    deadline::ptr inactivity_;

    // These are not thread safe, caller must be stranded.
    bool notify_on_connect_;
    uint64_t channel_nonce_;
    uint32_t negotiated_version_;
    messages::version::ptr peer_version_;
};

} // namespace network
} // namespace libbitcoin

#endif
