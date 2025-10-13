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
#include <bitcoin/network/net/channel_http.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

#define CLASS channel_http

using namespace system;
using namespace std::placeholders;

// Shared pointers required in handler parameters so closures control lifetime.
BC_PUSH_WARNING(NO_VALUE_OR_CONST_REF_SHARED_PTR)
BC_PUSH_WARNING(SMART_PTR_NOT_NEEDED)
BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

channel_http::channel_http(const logger& log, const socket::ptr& socket,
    const network::settings& settings, uint64_t identifier,
    const network::settings::http_server& options) NOEXCEPT
  : channel(log, socket, settings, identifier, options.timeout(), {}),
    request_buffer_(ceilinged_add(http::max_head, http::max_body)),
    distributor_(socket->strand()),
    tracker<channel_http>(log)
{
}

// Start/stop/resume (started upon create).
// ----------------------------------------------------------------------------

// This should not be called internally, as derived rely on stop() override.
void channel_http::stopping(const code& ec) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    channel::stopping(ec);
    distributor_.stop(ec);
}

void channel_http::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    channel::resume();
    read_request();
}

// Read cycle (read continues until stop called, call only once).
// ----------------------------------------------------------------------------

void channel_http::read_request() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!reading_, "already reading");

    // Both terminate read loop, paused can be resumed, stopped cannot.
    // Pause only prevents start of the read loop, it does not prevent messages
    // from being issued for sockets already past that point (e.g. waiting).
    // This is mainly for startup coordination, preventing missed messages.
    if (stopped() || paused() || reading_)
        return;

    // HTTP is half duplex.
    reading_ = true;
    const auto request = make_shared<http::string_request>();

    // Post handle_read_request to strand upon stop, error, or buffer full.
    read(request_buffer_, *request,
        std::bind(&channel_http::handle_read_request,
            shared_from_base<channel_http>(), _1, _2, request));
}

void channel_http::handle_read_request(const code& ec, size_t,
    const http::string_request_cptr& request) NOEXCEPT
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

    // HTTP is half duplex.
    reading_ = false;
    distributor_.notify(request);
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
