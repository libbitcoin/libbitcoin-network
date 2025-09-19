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
    buffer_(min_heading),
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

    read_request(zero);
}

void channel_client::read_request(size_t offset) NOEXCEPT
{
    BC_ASSERT_MSG(stranded(), "strand");

    // Post handle_read_heading to strand upon stop, error, or buffer full.
    read_some({ std::next(buffer_.begin(), offset), buffer_.end() },
        std::bind(&channel_client::handle_read_request,
            shared_from_base<channel_client>(), _1, _2, offset));
}

void channel_client::handle_read_request(const code& ec, size_t bytes_read,
    size_t offset) NOEXCEPT
{
    // Reliance on API correctness and protected invocation is sufficient.
    BC_ASSERT_MSG(stranded(), "strand");
    BC_ASSERT_MSG(!is_add_overflow(offset, bytes_read), "overflow");
    BC_ASSERT_MSG(buffer_.size() >= add(offset, bytes_read), "buffer overflow");

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

    const auto size = offset + bytes_read;
    const auto end = std::next(buffer_.cbegin(), size);
    const auto populated = std::ranges::subrange(buffer_.cbegin(), end);
    if (search(populated, heading::terminal).empty())
    {
        if (buffer_.size() == max_heading)
        {
            // TODO: notify subscribers with faulted request object.
            LOGR("Request oversized header [" << authority() << "]");
            stop(error::invalid_message);
            return;
        }

        static_assert(max_heading < to_half(max_size_t));
        buffer_.resize(limit(two * buffer_.size(), max_heading));
        read_request(size);
        return;
    }

    istream source{ buffer_ };
    byte_reader reader{ source };
    reader.set_limit(size);
    const auto request = to_shared(request::deserialize(reader));
    if (!reader)
    {
        // TODO: notify subscribers with faulted request object.
        LOGR("Request invalid header [" << authority() << "]");
        stop(error::invalid_message);
        return;
    }

    // reader is positioned to first body byte and limited to read bytes.
    // However more read is required and not limited to fixed buffer.
    // Also a second request may potentially arrive at the end of a first when
    // operating in full duplex mode, so we need to limit reads to predicted
    // lengths, either via content-length or iteration over chunked encoding.
    // We can reuse and increase the buffer size for each length/chunk, as
    // required, and reset (trim) after body completion. But we want contiguous
    // reads per chunk, cannot predict the request header length, and prefer to
    // not copy the partial body read to the buffer start before reading after
    // completing the header, since the body might already be read. So we can
    // just resize up the buffer if required, but this invalidates iterators.

    std::string out{};
    out.resize(request->size());
    if (request->serialize(out))
    {
        LOGA(out);
    }
}

BC_POP_WARNING()

} // namespace network
} // namespace libbitcoin
