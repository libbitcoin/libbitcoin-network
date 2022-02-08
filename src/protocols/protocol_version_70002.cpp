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
#include <bitcoin/network/protocols/protocol_version_70002.hpp>

#include <cstdint>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/p2p.hpp>
#include <bitcoin/network/protocols/protocol_version_31402.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_version_70002
static const std::string protocol_name = "version";

using namespace bc::system;
using namespace bc::network::messages;
using namespace std::placeholders;

static const std::string insufficient_version = "insufficient-version";
static const std::string insufficient_services = "insufficient-services";

// TODO: set explicitly on inbound (none or new config) and self on outbound.
// Configured services was one but we found that most incoming connections are
// set to zero, so that is currently the default (see below).
protocol_version_70002::protocol_version_70002(const session& session,
    channel::ptr channel)
  : protocol_version_70002(session, channel,
        session.settings().protocol_maximum,
        session.settings().services,
        session.settings().invalid_services,
        session.settings().protocol_minimum,
        service::node_none,
        /*session.settings().services,*/
        session.settings().relay_transactions)
{
}

protocol_version_70002::protocol_version_70002(const session& session,
    channel::ptr channel, uint32_t own_version, uint64_t own_services,
    uint64_t invalid_services, uint32_t minimum_version,
    uint64_t minimum_services, bool relay)
  : protocol_version_31402(session, channel, own_version, own_services,
        invalid_services, minimum_version, minimum_services),
    relay_(relay)
{
}

// Start sequence.
// ----------------------------------------------------------------------------

void protocol_version_70002::start(result_handler handle_event)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    protocol_version_31402::start(handle_event);
    SUBSCRIBE2(reject, handle_receive_reject, _1, _2);
}

version protocol_version_70002::version_factory() const
{
    // Relay is the only difference at protocol level 70001.
    auto version = protocol_version_31402::version_factory();
    version.relay = relay_;
    return version;
}

// Protocol.
// ----------------------------------------------------------------------------

bool protocol_version_70002::sufficient_peer(version::ptr message)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (message->value < minimum_version_)
    {
        SEND2((reject{ version::command, reject::reason_code::obsolete }),
            handle_send, _1, reject::command);
    }
    else if ((message->services & minimum_services_) != minimum_services_)
    {
        SEND2((reject { version::command, reject::reason_code::obsolete }),
            handle_send, _1, reject::command);
    }

    return protocol_version_31402::sufficient_peer(message);
}

void protocol_version_70002::handle_receive_reject(const code& ec,
    reject::ptr reject)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving reject from [" << authority() << "] "
            << ec.message() << std::endl;
        set_event(error::channel_stopped);
        return;
    }

    const auto& message = reject->message;

    // Handle these in the reject protocol.
    if (message != version::command)
        return;

    const auto code = reject->code;

    // Client is an obsolete, unsupported version.
    if (code == reject::reason_code::obsolete)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Obsolete version reject from [" << authority() << "] '"
            << reject->reason << "'" << std::endl;
        set_event(error::channel_stopped);
        return;
    }

    // Duplicate version message received.
    if (code == reject::reason_code::duplicate)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Duplicate version reject from [" << authority() << "] '"
            << reject->reason << "'" << std::endl;
        set_event(error::channel_stopped);
    }
}

const std::string& protocol_version_70002::name() const
{
    return protocol_name;
}

} // namespace network
} // namespace libbitcoin
