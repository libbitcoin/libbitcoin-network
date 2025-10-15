/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_SESSION_SERVER_HPP
#define LIBBITCOIN_NETWORK_SESSION_SERVER_HPP

#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/sessions/session_tcp.hpp>

namespace libbitcoin {
namespace network {

class net;

// make_shared<>
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

/// Client-server connections session template, thread safe.
/// Declare a concrete instance of this type for client-server protocols built
/// on tcp/ip. Base class processing performs all connection management and
/// session tracking. This includes start/stop/disable/enable/black/whitelist.
/// Protocol must declare options_t and channel_t. This protocol is constructed
/// and attached to a constructed instance of channel_t. The protocol construct
/// and attachment can be overridden and/or augmented with other protocols.
template <typename Protocol, typename Session = session_tcp>
class session_server
  : public Session, protected tracker<session_server<Protocol, Session>>
{
public:
    typedef std::shared_ptr<session_server<Protocol, Session>> ptr;

    /// The protocol must define these public types.
    using options_t = typename Protocol::options_t;
    using channel_t = typename Protocol::channel_t;

    /// Construct an instance (network should be started).
    /// The options reference must be kept in scope, the string name is copied.
    template <typename Network>
    session_server(Network& network, uint64_t identifier,
        const options_t& options) NOEXCEPT
      : Session(network, identifier, options),
        options_(options), tracker<session_server<Protocol, Session>>(network)
    {
    }

protected:
    /// Override to construct channel. This allows the implementation to pass
    /// other values to protocol construction and/or select the desired channel
    /// based on available factors (e.g. a distinct protocol version).
    inline channel::ptr create_channel(
        const socket::ptr& socket) NOEXCEPT override
    {
        BC_ASSERT_MSG(this->stranded(), "strand");

        return std::make_shared<channel_t>(this->log, socket,
            this->settings(), this->create_key(), options_);
    }

    /// Override to implement a connection handshake as required. By default
    /// this is bypassed, which applies to basic http services. A handshake
    /// is used to implement TLS and WebSocket upgrade from http (for example).
    /// Handshake protocol(s) must invoke handler one time at completion.
    /// Use std::dynamic_pointer_cast<channel_t>(channel) to obtain channel_t.
    inline void attach_handshake(const channel::ptr& channel,
        result_handler&& handler) NOEXCEPT override
    {
        BC_ASSERT_MSG(channel->stranded(), "channel strand");
        BC_ASSERT_MSG(channel->paused(), "channel not paused for handshake");

        this->attach_handshake(channel, std::move(handler));
    }

    /// Overridden to set channel protocols. This allows the implementation to
    /// pass other values to protocol construction and/or select the desired
    /// protocol based on available factors (e.g. a distinct protocol version).
    /// Use std::dynamic_pointer_cast<channel_t>(channel) to obtain channel_t.
    inline void attach_protocols(const channel::ptr& channel) NOEXCEPT override
    {
        BC_ASSERT_MSG(channel->stranded(), "channel strand");
        BC_ASSERT_MSG(channel->paused(), "channel not paused for protocols");

        const auto self = this->template shared_from_base<Session>();
        channel->attach<Protocol>(self, options_)->start();
    }

private:
    // This is thread safe.
    const options_t& options_;
};

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin

#endif
