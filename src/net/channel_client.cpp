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
#include <bitcoin/network/net/channel_client.hpp>

#include <algorithm>
#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define CLASS channel_client

using namespace system;
using namespace messages::rpc;
using namespace std::placeholders;
using namespace std::ranges;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// TODO: inactivity timeout, duration timeout, connection limit.
channel_client::channel_client(const logger& log, const socket::ptr& socket,
    const network::settings& settings, uint64_t identifier) NOEXCEPT
  : channel(log, socket, settings, identifier),
    request_buffer_(ceilinged_add(max_head, max_body)),
    subscriber_(strand()),
    tracker<channel_client>(log)
{
}

// Start/stop/resume (started upon create).
// ----------------------------------------------------------------------------

void channel_client::stop(const code& ec) NOEXCEPT
{
    // Stop the read loop, stop accepting new work, cancel pending work.
    channel::stop(ec);

    // Stop is posted to strand to protect timers.
    boost::asio::post(strand(),
        std::bind(&channel_client::do_stop,
            shared_from_base<channel_client>(), ec));
}

// This should not be called internally, as derived rely on stop() override.
void channel_client::do_stop(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    subscriber_.stop(ec, {});
}

// Resume must be called (only once) from message handler (if not stopped).
// Calling more than once is safe but implies a protocol problem. Failure to
// call after successful message handling will result in a stalled channel.
void channel_client::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (paused())
    {
        channel::resume();
        read_request();
    }
}

// Read cycle (read continues until stop called, call only once).
// ----------------------------------------------------------------------------

void channel_client::read_request() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Both terminate read loop, paused can be resumed, stopped cannot.
    // Pause only prevents start of the read loop, it does not prevent messages
    // from being issued for sockets already past that point (e.g. waiting).
    // This is mainly for startup coordination, preventing missed messages.
    if (stopped() || paused())
        return;

    const auto request = to_shared<http_string_request>();

    // 'prepare' appends available to write portion of buffer (moves pointers).
    read(request_buffer_, *request,
        std::bind(&channel_client::handle_read_request,
            shared_from_base<channel_client>(), _1, _2, request));
}

void channel_client::handle_read_request(const code& ec, size_t,
    const http_string_request_ptr& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Request read abort [" << authority() << "]");
        return;
    }

    if (ec)
    {
        // Don't log common conditions.
        if (ec != error::peer_disconnect && ec != error::operation_canceled)
        {
            LOGF("Request read failure [" << authority() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    // HTTP is half duplex, this subscriber must stop or resume channel.
    pause();

    subscriber_.notify(error::success, *request);
}

// return error::beast_to_error_code(ec);

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
