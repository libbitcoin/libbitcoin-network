/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_SESSION_BATCH_HPP
#define LIBBITCOIN_NETWORK_SESSION_BATCH_HPP

#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/channel.hpp>
#include <bitcoin/network/connector.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/sessions/session.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class p2p;

/// Intermediate base class for adding batch connect sequence.
class BCT_API session_batch
  : public session
{
protected:

    /// Construct an instance.
    session_batch(p2p& network, bool notify_on_connect);

    /// Create a channel from the configured number of concurrent attempts.
    virtual void connect(channel_handler handler);

private:
    // Connect sequence
    void new_connect(channel_handler handler);
    void start_connect(const code& ec, const authority& host,
        channel_handler handler);
    void handle_connect(const code& ec, channel::ptr channel,
        connector::ptr connector, channel_handler handler);

    const size_t batch_size_;
};

} // namespace network
} // namespace libbitcoin

#endif
