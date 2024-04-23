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
#include <bitcoin/network/protocols/protocol_alert_31402.hpp>

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>
#include <bitcoin/network/protocols/protocol.hpp>
#include <bitcoin/network/sessions/sessions.hpp>

namespace libbitcoin {
namespace network {

#define CLASS protocol_alert_31402

using namespace system;
using namespace messages;
using namespace std::placeholders;

// This captures alert messages. Outp
protocol_alert_31402::protocol_alert_31402(const session::ptr& session,
    const channel::ptr& channel) NOEXCEPT
  : protocol(session, channel),
    tracker<protocol_alert_31402>(session->log)
{
}

// Start.
// ----------------------------------------------------------------------------

void protocol_alert_31402::start() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_alert_31402");

    if (started())
        return;

    SUBSCRIBE_CHANNEL(alert, handle_receive_alert, _1, _2);

    protocol::start();
}

// Inbound (log alert).
// ----------------------------------------------------------------------------

bool protocol_alert_31402::handle_receive_alert(const code& ec,
    const alert::cptr& LOG_ONLY(alert)) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "protocol_alert_31402");

    if (stopped(ec))
        return false;

    // TODO: serialize cancels and sub_versions.
    // Signature not validated because is not relevant (private key published).
    LOGN("Alert from [" << authority() << "]..."
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
        << "\nsignature   : " << system::serialize(alert->signature));

    return true;
}

} // namespace network
} // namespace libbitcoin
