/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/protocols/protocol_version_70001.hpp>

#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol_version_31402.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_version_70001

using namespace system;
using namespace bc::network::messages;

protocol_version_70001::protocol_version_70001(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol_version_70001(session, channel,
        session->settings().services_minimum,
        session->settings().services_maximum,
        session->settings().enable_transaction)
{
}

protocol_version_70001::protocol_version_70001(const session::ptr& session,
    const channel::ptr& channel, uint64_t minimum_services,
    uint64_t maximum_services, bool relay) NOEXCEPT
  : protocol_version_31402(session, channel, minimum_services,
      maximum_services),
    relay_(relay),
    tracker<protocol_version_70001>(session->log)
{
}

// Utilities.
// ----------------------------------------------------------------------------
// Relay is the only difference at protocol level 70001.

messages::version protocol_version_70001::version_factory(bool) const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70001");
    return protocol_version_31402::version_factory(relay_);
}

} // namespace network
} // namespace libbitcoin
