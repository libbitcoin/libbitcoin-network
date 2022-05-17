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
#include <bitcoin/network/protocols/protocol.hpp>

#include <cstdint>
#include <string>
#include <utility>
#include <boost/asio.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

protocol::protocol(const session& session, const channel::ptr& channel)
  : channel_(channel), session_(session), started_(false)
{
}

protocol::~protocol()
{
    BC_ASSERT_MSG(stopped(), "protocol destruct before channel stop");
}

// Start/Stop.
// ----------------------------------------------------------------------------

void protocol::start()
{
    BC_ASSERT_MSG(stranded(), "stranded");
    started_ = true;
}

bool protocol::started() const
{
    BC_ASSERT_MSG(stranded(), "stranded");
    return started_;
}

// Utility to test for failure code or stopped.
// Any failure code from a send or receive handler implies channel::proxy stop.
// So this should be used to test entry into those handlers. Other handlers,
// such as timers, should use stopped() and independently evaluate the code.
bool protocol::stopped(const code& ec) const
{
    return channel_->stopped() || ec;
}

// Called from stop subscription instead of stop (which would be a cycle).
void protocol::stopping(const code& ec)
{
    BC_ASSERT_MSG(stranded(), "stranded");
}

// Stop the channel::proxy, which results protocol stop handler invocation.
// The stop handler invokes stopping(ec), for protocol cleanup operations.
void protocol::stop(const code& ec)
{
    channel_->stop(ec);
}

// Properties.
// ----------------------------------------------------------------------------
// These public properties may be accessed outside the strand, but are never
// during handshake protocol operation. Thread safety requires that setters are
// never invoked outside of the handshake protocol (start handler).

bool protocol::stranded() const
{
    return channel_->stranded();
}

config::authority protocol::authority() const
{
    return channel_->authority();
}

uint64_t protocol::nonce() const noexcept
{
    return channel_->nonce();
}

const network::settings& protocol::settings() const
{
    return session_.settings();
}

version::ptr protocol::peer_version() const noexcept
{
    return channel_->peer_version();
}

// Call only from handshake (version protocol), for thread safety.
void protocol::set_peer_version(const version::ptr& value) noexcept
{
    channel_->set_peer_version(value);
}

uint32_t protocol::negotiated_version() const noexcept
{
    return channel_->negotiated_version();
}

// Call only from handshake (version protocol), for thread safety.
void protocol::set_negotiated_version(uint32_t value) noexcept
{
    channel_->set_negotiated_version(value);
}

// Addresses.
// ----------------------------------------------------------------------------
// Address completion handlers are invoked on the channel strand.

void protocol::fetches(fetches_handler&& handler)
{
    session_.fetches(BIND3(do_fetches, _1, _2, std::move(handler)));
}

// Return to channel strand.
void protocol::do_fetches(const code& ec,
    const messages::address_items& addresses, const fetches_handler& handler)
{
    // TODO: use addresses pointer (copies addresses).
    boost::asio::post(channel_->strand(),
        BIND3(handle_fetches, ec, addresses, handler));
}

void protocol::handle_fetches(const code& ec,
    const messages::address_items& addresses, const fetches_handler& handler)
{
    BC_ASSERT_MSG(stranded(), "protocol");

    // TODO: log code here for derived protocols.
    ////LOG_DEBUG(LOG_NETWORK)
    ////    << "Fetched addresses for [" << authority() << "] ("
    ////    << addresses.size() << ")" << std::endl;

    handler(ec, addresses);
}

void protocol::saves(const messages::address_items& addresses)
{
    const auto self = shared_from_base<protocol>();
    return saves(addresses, [self](const code&)
    {
        BC_ASSERT_MSG(self->stranded(), "protocol");
        self->nop();
    });
}

void protocol::saves(const messages::address_items& addresses,
    result_handler&& handler)
{
    ////LOG_VERBOSE(LOG_NETWORK)
    ////    << "Storing addresses from [" << authority() << "] ("
    ////    << addresses.size() << ")" << std::endl;

    // TODO: use addresses pointer (copies addresses).
    session_.saves(addresses, BIND2(do_saves, _1, std::move(handler)));
}

// Return to channel strand.
void protocol::do_saves(const code& ec, const result_handler& handler)
{
    boost::asio::post(channel_->strand(),
        BIND2(handle_saves, ec, std::move(handler)));
}

void protocol::handle_saves(const code& ec, const result_handler& handler)
{
    BC_ASSERT_MSG(stranded(), "protocol");

    // TODO: log code here for derived protocols.
    handler(ec);
}

// Send.
// ----------------------------------------------------------------------------
// Send (and receive) completion handlers are invoked on the channel strand.

// Send and receive failures are logged by the proxy, so there is no need to
// log here. This can be used as a no-op handler for sends. Some protocols may
// create custom handlers to perform operations upon send completion.
void protocol::handle_send(const code&)
{
    BC_ASSERT_MSG(stranded(), "strand");
}

} // namespace network
} // namespace libbitcoin
