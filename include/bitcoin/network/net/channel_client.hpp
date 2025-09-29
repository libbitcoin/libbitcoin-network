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
#include <utility>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/settings.hpp>
#include <bitcoin/network/net/channel.hpp>

namespace libbitcoin {
namespace network {

class BCT_API channel_client
  : public channel, protected tracker<channel_client>
{
public:
    typedef subscriber<asio::http_request> request_subscriber;
    typedef request_subscriber::handler request_notifier;

    typedef std::shared_ptr<channel_client> ptr;

    /// Subscribe to channel request messages (use SUBSCRIBE_CHANNEL).
    /// Method is invoked with error::subscriber_stopped if already stopped.
    template <class Message, typename Handler = asio::http_request,
        if_same<Message, asio::http_request> = true>
    void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        subscriber_.subscribe(std::forward<Handler>(handler));
    }

    /// Send a message instance to peer (use SEND).
    template <class Message, if_same<Message, asio::http_response> = true>
    void send(const Message&, result_handler&& complete) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        // TODO: serialize message to data chunk.
        const auto data = std::make_shared<system::data_chunk>();

        if (!data)
        {
            // This is an internal error, should never happen.
            LOGF("Serialization failure (http_response).");
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

private:
    void read_request() NOEXCEPT;
    code parse_buffer(size_t bytes_read) NOEXCEPT;
    void handle_read_request(const code& ec, size_t bytes_read) NOEXCEPT;
    void do_stop(const code& ec) NOEXCEPT;

    request_subscriber subscriber_;
    asio::http_buffer buffer_;
    asio::http_parser parser_{};
};

} // namespace network
} // namespace libbitcoin

#endif
