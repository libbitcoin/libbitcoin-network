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
#include <bitcoin/network/protocols/protocol_version_70016.hpp>

#include <utility>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/p2p/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_version_70002.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

// sendaddrv2 is a a broken protocol in that it is a formally unversioned
// message, though Satoshi doesn't send until receiving version >= 70016.
// It must be sent/received before verack (and sent after version receipt).

namespace libbitcoin {
namespace network {

#define CLASS protocol_version_70016

using namespace system;
using namespace network::messages::p2p;
using namespace std::placeholders;

protocol_version_70016::protocol_version_70016(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol_version_70016(session, channel,
        session->settings().services_minimum,
        session->settings().services_maximum,
        session->settings().enable_relay,
        session->settings().enable_reject)
{
}

protocol_version_70016::protocol_version_70016(const session::ptr& session,
    const channel::ptr& channel,
    uint64_t minimum_services,
    uint64_t maximum_services,
    bool relay,
    bool reject) NOEXCEPT
  : protocol_version_70002(session, channel, minimum_services, maximum_services, relay),
    reject_(reject),
    tracker<protocol_version_70016>(session->log)
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_version_70016::shake(result_handler&& handle_event) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70016");

    if (started())
        return;

    // Protocol versions are cumulative, but reject is optional.
    if (reject_)
    {
        protocol_version_70002::shake(std::move(handle_event));
        return;
    }
    
    protocol_version_70001::shake(std::move(handle_event));
}

// Incoming [send_address_v2    => negotiated state change].
// Incoming [witness_tx_id_relay => negotiated state change].
// ----------------------------------------------------------------------------

void protocol_version_70016::handle_send_version(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70016");

    SUBSCRIBE_CHANNEL(send_address_v2, handle_receive_send_address_v2, _1, _2);
    SUBSCRIBE_CHANNEL(witness_tx_id_relay, handle_receive_witness_tx_id_relay, _1, _2);
    protocol_version_70002::handle_send_version(ec);
}

bool protocol_version_70016::handle_receive_acknowledge(const code& ec,
    const messages::p2p::version_acknowledge::cptr& message) NOEXCEPT
{
    complete_ = true;

    // Channel will pause after this and then be restarted as connected.
    return protocol_version_70002::handle_receive_acknowledge(ec, message);
}

bool protocol_version_70016::handle_receive_send_address_v2(const code& ec,
    const send_address_v2::cptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70016");

    if (stopped(ec))
        return false;

    // Late send_address_v2.
    if (complete_)
    {
        rejection(error::protocol_violation);
        return false;
    }

    // TODO: set channel send_address_v2 property and use to attach protocols.
    return true;
}

bool protocol_version_70016::handle_receive_witness_tx_id_relay(const code& ec,
    const witness_tx_id_relay::cptr&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70016");

    if (stopped(ec))
        return false;

    // Late witness_tx_id_relay.
    if (complete_)
    {
        rejection(error::protocol_violation);
        return false;
    }

    // TODO: set channel witness_tx_id_relay_relay property and use to attach protocols.
    return true;
}

} // namespace network
} // namespace libbitcoin
