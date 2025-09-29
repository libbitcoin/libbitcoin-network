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
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace messages::rpc;
using namespace std::placeholders;
using namespace std::ranges;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

channel_client::channel_client(const logger& log, const socket::ptr& socket,
    const network::settings& settings, uint64_t identifier) NOEXCEPT
  : channel(log, socket, settings, identifier),
    distributor_(socket->strand()),
    buffer_{ max_head + max_body },
    tracker<channel_client>(log)
{
    parser_.header_limit(max_head);
    parser_.body_limit(max_body);
}

// Stop (started upon create).
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

    distributor_.stop(ec);
}

// Pause/resume (paused upon create).
// ----------------------------------------------------------------------------

void channel_client::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    channel::resume();
    read_resume();
}

// Read cycle (read continues until stop called, call only once).
// ----------------------------------------------------------------------------

void channel_client::read_resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(is_zero(buffer_.size()), "call read_resume only once");

    if (stopped() || paused())
        return;

    read_request();
}

void channel_client::read_request() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Limit set on construct, does not result in allocation.
    const auto size   = buffer_.size();
    const auto limit  = buffer_.max_size();
    const auto remain = floored_subtract(limit, size);

    // TODO: read_some(mutable_buffer buffer,...) allows for simplification of
    // TODO: read_some(buffer_.prepare(remain),...)
    const auto buffer = buffer_.prepare(remain);
    const auto begin  = pointer_cast<uint8_t>(buffer.data());
    const auto end    = std::next(begin, remain);

    // Post handle_read_heading to strand upon stop, error, or buffer full.
    read_some({ begin, end },
        std::bind(&channel_client::handle_read_request,
            shared_from_base<channel_client>(), _1, _2));
}

void channel_client::handle_read_request(const code& ec,
    size_t bytes_read) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped())
    {
        LOGQ("Request read abort [" << authority() << "]");
        stop(error::channel_stopped);
        return;
    }

    if (ec)
    {
        if (ec != error::peer_disconnect && ec != error::operation_canceled)
        {
            LOGF("Request read failure [" << authority() << "] "
                << ec.message());
        }

        stop(ec);
        return;
    }

    // Commits read bytes_read to buffer size.
    const auto code = parse_buffer(bytes_read);

    if (code == error::need_more)
    {
        // Wait on more bytes for this request.
        read_request();
        return;
    }

    if (code)
    {
        LOGR("Request parse error [" << authority() << "] " << code.message());
    }

    // Clears buffer (size set to zero).
    // Failure responses are sent by handler (followed by stop).
    notify(code, parser_.release());

    // Wait on next request.
    read_request();
}

// put() will not return http::error::need_more if either the header_limit or
// body_limit has been reached (returns other specific error code).
code channel_client::parse_buffer(size_t bytes_read) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    using namespace boost::asio;
    using namespace boost::beast;

    error_code result{};
    buffer_.commit(bytes_read);

    try
    {
        parser_.put({ buffer_.data() }, result);
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Request parse exception [" << authority() << "] " << e.what());
        result = http::error::bad_alloc;
    }

    return error::beast_to_error_code(result);
}

code channel_client::notify(const code&,
    const asio::http_request&) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // These correspond to libbitcoin request.
    ////const auto target = request.target();
    ////const auto method = request.method();
    ////const auto version = request.version();
    ////const auto& field = request["field-name"];
    ////const auto& body = request.body();

    // TODO: process and distribute request.
    //// distributor_.notify(id, source);

    // Invalidates the above parser reference (cannot be passed on above).
    buffer_.consume(buffer_.size());
    return error::success;
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
