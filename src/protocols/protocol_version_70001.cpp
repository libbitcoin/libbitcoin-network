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
#include <bitcoin/network/protocols/protocol_version_70001.hpp>

#include <cstdint>
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

#define CLASS protocol_version_70001

using namespace bc::system;
using namespace bc::network::messages;

static const std::string protocol_name = "version";

protocol_version_70001::protocol_version_70001(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol_version_70001(session, channel,
        session.settings().services_minimum,
        session.settings().services_maximum,
        session.settings().relay_transactions)
{
}

protocol_version_70001::protocol_version_70001(const session& session,
    const channel::ptr& channel, uint64_t minimum_services,
    uint64_t maximum_services, bool relay) NOEXCEPT
  : protocol_version_31402(session, channel, minimum_services,
      maximum_services),
    relay_(relay)
{
}

const std::string& protocol_version_70001::name() const NOEXCEPT
{
    return protocol_name;
}

// Utilities.
// ----------------------------------------------------------------------------

protocol_version_70001::version_ptr
protocol_version_70001::version_factory() const NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_version_70001");

    const auto version = protocol_version_31402::version_factory();

    // Relay is the only difference at protocol level 70001.
    version->relay = relay_;
    return version;
}

} // namespace network
} // namespace libbitcoin
