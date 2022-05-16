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

#include <string>
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_version_31402.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_version_70002

using namespace bc::system;
using namespace bc::network::messages;
using namespace std::placeholders;

static const std::string protocol_name = "version";
static const std::string insufficient_version = "insufficient-version";
static const std::string insufficient_services = "insufficient-services";

protocol_version_70002::protocol_version_70002(const session& session,
    const channel::ptr& channel)
  : protocol_version_70002(session, channel,
        session.settings().services_minimum,
        session.settings().services_maximum,
        session.settings().relay_transactions)
{
}

protocol_version_70002::protocol_version_70002(const session& session,
    const channel::ptr& channel, uint64_t minimum_services,
    uint64_t maximum_services, bool relay)
  : protocol_version_31402(session, channel, minimum_services,
      maximum_services),
    relay_(relay)
{
}

const std::string& protocol_version_70002::name() const
{
    return protocol_name;
}

// Utilities.
// ----------------------------------------------------------------------------

version protocol_version_70002::version_factory() const
{
    // Relay is the only difference at protocol level 70001.
    auto version = protocol_version_31402::version_factory();
    version.relay = relay_;
    return version;
}

// Start.
// ----------------------------------------------------------------------------

void protocol_version_70002::start(result_handler&& handle_event)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (started())
        return;

    SUBSCRIBE2(reject, handle_receive_reject, _1, _2);

    protocol_version_31402::start(std::move(handle_event));
}

// Protocol.
// ----------------------------------------------------------------------------

bool protocol_version_70002::sufficient_peer(const version::ptr& message)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (message->value < minimum_version_)
    {
        SEND1((reject{ version::command, reject::reason_code::obsolete }),
            handle_send, _1);
    }
    else if ((message->services & minimum_services_) != minimum_services_)
    {
        SEND1((reject { version::command, reject::reason_code::obsolete }),
            handle_send, _1);
    }

    return protocol_version_31402::sufficient_peer(message);
}

void protocol_version_70002::handle_receive_reject(const code& ec,
    const reject::ptr& reject)
{
    BC_ASSERT_MSG(stranded(), "stranded");

    if (stopped(ec))
        return;

    if (ec)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Failure receiving reject from [" << authority() << "] "
            << ec.message() << std::endl;
        ////set_event(error::channel_stopped);
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
        ////set_event(error::channel_stopped);
        return;
    }

    // Duplicate version message received.
    if (code == reject::reason_code::duplicate)
    {
        LOG_DEBUG(LOG_NETWORK)
            << "Duplicate version reject from [" << authority() << "] '"
            << reject->reason << "'" << std::endl;
        ////set_event(error::channel_stopped);
    }
}

} // namespace network
} // namespace libbitcoin
