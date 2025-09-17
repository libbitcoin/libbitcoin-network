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
#ifndef LIBBITCOIN_NETWORK_NET_CHANNEL_CLIENT_HPP
#define LIBBITCOIN_NETWORK_NET_CHANNEL_CLIENT_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/net/distributor_client.hpp>

namespace libbitcoin {
namespace network {

class BCT_API channel_client
  : public channel, protected tracker<channel_client>
{
public:
    typedef std::shared_ptr<channel_client> ptr;

    /// Subscribe to messages from client (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Message,
        typename Handler = distributor_client::handler<Message>>
    void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        distributor_.subscribe(std::forward<Handler>(handler));
    }

    /// Serialize and write a message to the client (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <class Message>
    void send(const Message& message, result_handler&& complete) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        ///////////////////////////////////////////////////////////////////////
        // TODO: update serialize to include status line and headers.
        ///////////////////////////////////////////////////////////////////////
        const auto data = messages::rpc::serialize(message);

        if (!data)
        {
            // This is an internal error, should never happen.
            LOGF("Serialization failure (" << Message::command << ").");
            complete(error::unknown);
            return;
        }

        write(data, std::move(complete));
    }

    /// Construct client channel to encapsulate and communicate on the socket.
    channel_client(const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero) NOEXCEPT;

    /// Idempotent, may be called multiple times.
    void stop(const code& ec) NOEXCEPT override;

    /// Resume reading from the socket (requires strand).
    void resume() NOEXCEPT override;

protected:
    /// Client read and dispatch.
    void read_request() NOEXCEPT;
    void handle_read_request(const code& ec, size_t) NOEXCEPT;

    /// Notify subscribers of a new message (requires strand).
    virtual code notify(messages::rpc::identifier id,
        const system::data_chunk& source) NOEXCEPT;

private:
    void do_stop(const code& ec) NOEXCEPT;

    distributor_client distributor_;
    system::data_chunk buffer_{};
};

} // namespace network
} // namespace libbitcoin

#endif
