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

channel_client::channel_client(const logger& log, const socket::ptr& socket,
    const network::settings& settings, uint64_t identifier) NOEXCEPT
  : channel(log, socket, settings, identifier),
    subscriber_(strand()),
    buffer_(ceilinged_add(max_head, max_body)),
    tracker<channel_client>(log)
{
    initialize(parser_);
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

void channel_client::resume() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    channel::resume();
    read_request();
}

// Read cycle (read continues until stop called, call only once).
// ----------------------------------------------------------------------------

void channel_client::read_request() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Post handle_read_heading to strand upon stop, error, or buffer full.
    read_some(buffer_.prepare(buffer_.max_size() - buffer_.size()),
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

    buffer_.commit(bytes_read);
    const auto code = parse(buffer_);

    if (code != error::need_more)
    {
        if (code)
        {
            LOGR("Invalid request [" << authority() << "] " << code.message());
        }

        subscriber_.notify(code, detach(parser_));
        buffer_.consume(buffer_.size());
    }

    // Wait on more bytes for this request (if need_more), or for next request.
    read_request();
}

// Parser utilities.
// ----------------------------------------------------------------------------
// static

void channel_client::initialize(http_parser_ptr& parser) NOEXCEPT
{
    parser = std::make_unique<asio::http_parser>();
    parser->header_limit(max_head);
    parser->body_limit(max_body);
}

asio::http_request channel_client::detach(http_parser_ptr& parser) NOEXCEPT
{
    BC_ASSERT(parser);
    auto out = parser->release();
    initialize(parser);
    return out;
}

// Handles exceptions, error code conversion, and const_buffer wrapping.
code channel_client::parse(const asio::http_buffer& buffer) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    using namespace boost::beast;
    error_code result{};

    try
    {
        parser_->put(asio::const_buffer{ buffer.data() }, result);
    }
    catch (const std::exception& LOG_ONLY(e))
    {
        LOGF("Request parse exception [" << authority() << "] " << e.what());
        result = http::error::bad_alloc;
    }

    return error::beast_to_error_code(result);
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
