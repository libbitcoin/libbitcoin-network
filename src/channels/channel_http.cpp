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
#include <bitcoin/network/channels/channel_http.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/http/http.hpp>
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
    // Pause is mainly for startup coordination, preventing missed messages.
    if (stopped() || paused() || reading_)
        return;

    // HTTP is half duplex.
    reading_ = true;
    const auto request = to_shared<http::request>();

    // Post handle_read_request to strand upon stop, error, or buffer full.
    read(request_buffer_, *request,
        std::bind(&channel_http::handle_read_request,
            shared_from_base<channel_http>(), _1, _2, request));
}

void channel_http::handle_read_request(const code& ec, size_t,
    const http::request_cptr& request) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // HTTP is half duplex.
    reading_ = false;

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

    log_message(*request);
    distributor_.notify(request);
}

// log helpers
// ----------------------------------------------------------------------------

void channel_http::log_message(const http::request& request) const NOEXCEPT
{
    LOG_ONLY(const auto size = serialize(request.payload_size()
        .has_value() ? request.payload_size().value() : zero);)

    LOG_ONLY(const auto version = "http/" + serialize(request.version() / 10) +
        "." + serialize(request.version() % 10);)
        
    LOGP("Request [" << request.method_string()
        << "] " << version << " (" << (request.chunked() ? "c" : size)
        << ") " << (request.keep_alive() ? "keep" : "drop")
        << " [" << authority() << "]"
        << " {" << (split(request[http::field::accept], ",").front()) << "...}"
        << " "  << request.target());
}

void channel_http::log_message(const http::response& response) const NOEXCEPT
{
    LOG_ONLY(const auto size = serialize(response.payload_size()
        .has_value() ? response.payload_size().value() : zero);)

    LOG_ONLY(const auto version = "http/" + serialize(response.version() / 10)
        + "." + serialize(response.version() % 10);)
        
    LOGP("Response [" << http::status_string(response.result())
        << "] " << version << " (" << (response.chunked() ? "c" : size)
        << ") " << (response.keep_alive() ? "keep" : "drop")
        << " [" << authority() << "]"
        << " {" << (response[http::field::content_type]) << "}");
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
