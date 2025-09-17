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

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

channel_client::channel_client(const logger& log, const socket::ptr& socket,
    const network::settings& settings, uint64_t identifier) NOEXCEPT
  : channel(log, socket, settings, identifier),
    distributor_(socket->strand()),
    tracker<channel_client>(log)
{
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
    read_request();
}

// Read cycle (read continues until stop called).
// ----------------------------------------------------------------------------

code channel_client::notify(messages::rpc::identifier id,
    const system::data_chunk& source) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");
    return distributor_.notify(id, source);
}

void channel_client::read_request() NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    if (stopped() || paused())
        return;

    // TODO: dynamic or fixed.
    buffer_.resize(messages::rpc::max_request);

    // Post handle_read_heading to strand upon stop, error, or buffer full.
    read_some(buffer_,
        std::bind(&channel_client::handle_read_request,
            shared_from_base<channel_client>(), _1, _2));
}

void channel_client::handle_read_request(const code& ec, size_t) NOEXCEPT
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

    const auto request = messages::rpc::request::deserialize(buffer_);

    if (!request)
    {
        // TODO: notify subscribers with faulted request object.
        LOGR("Request invalid or oversized header [" << authority() << "]");
        stop(error::invalid_message);
        return;
    }

    ////std::string out{};
    ////out.resize(request->size());
    ////if (request->serialize(out))
    ////{
    ////    LOGA(out);
    ////}
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
