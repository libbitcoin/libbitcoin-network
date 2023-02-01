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
#include <bitcoin/network/protocols/protocol_alert_31402.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_alert_31402

using namespace bc::system;
using namespace messages;
using namespace std::placeholders;

// This captures alert messages. Outp
protocol_alert_31402::protocol_alert_31402(const session& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    tracker<protocol_alert_31402>(session.log())
{
}

const std::string& protocol_alert_31402::name() const NOEXCEPT
{
    static const std::string protocol_name = "alert";
    return protocol_name;
}

// Start.
// ----------------------------------------------------------------------------

void protocol_alert_31402::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_alert_31402");

    if (started())
        return;

    SUBSCRIBE2(alert, handle_receive_alert, _1, _2);

    protocol::start();
}

// Inbound (log alert).
// ----------------------------------------------------------------------------

void protocol_alert_31402::handle_receive_alert(const code& ec,
    const alert::ptr& LOG_ONLY(alert)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_alert_31402");

    if (stopped(ec))
        return;

    // TODO: verify the signature (legacy).
    // TODO: serialize cancels and sub_versions.

    LOG("Alert from [" << authority() << "]..."
        << "\nversion     : " << alert->payload.version
        << "\nrelay_until : " << alert->payload.relay_until
        << "\nexpiration  : " << alert->payload.expiration
        << "\nid          : " << alert->payload.id
        << "\ncancel      : " << alert->payload.cancel
        << "\ncancels     : " << alert->payload.cancels.size()
        << "\nmin_version : " << alert->payload.min_version
        << "\nmax_version : " << alert->payload.max_version
        << "\nsub_versions: " << alert->payload.sub_versions.size()
        << "\npriority    : " << alert->payload.priority
        << "\ncomments    : " << alert->payload.comment.size()
        << "\nstatus_bar  : " << alert->payload.status_bar
        << "\nsignature   : " << system::serialize(alert->signature)
        << "\ncode        : " << ec.message());
}

} // namespace network
} // namespace libbitcoin
