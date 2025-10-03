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

#include <algorithm>
#include <memory>
#include <utility>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>
#include <bitcoin/network/net/channel.hpp>
#include <bitcoin/network/net/socket.hpp>
#include <bitcoin/network/settings.hpp>

namespace libbitcoin {
namespace network {

class BCT_API channel_client
  : public channel, protected tracker<channel_client>
{
public:
    typedef subscriber<http_string_request> request_subscriber;
    typedef request_subscriber::handler request_notifier;

    typedef std::shared_ptr<channel_client> ptr;

    /// Subscribe to http_request from peer (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Message, typename Handler,
        if_same<Message, http_string_request> = true>
    void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        subscriber_.subscribe(std::forward<Handler>(handler));
    }

    /// Serialize and write http response to peer (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <class Message>
    void send(Message&& response, result_handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");

        using namespace system;
        const auto ptr = make_shared(std::forward<Message>(response));
        auto complete = [ptr, handler](const code& ec, size_t) NOEXCEPT
        {
            handler(ec);
        };

        write(*ptr, std::move(complete));
    }

    /// Construct client channel to encapsulate and communicate on the socket.
    channel_client(const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero) NOEXCEPT;

    /// Idempotent, may be called multiple times.
    void stop(const code& ec) NOEXCEPT override;

    /// Resume reading from the socket (requires strand).
    void resume() NOEXCEPT override;

private:
    // Parser utilities.
    static http_string_request detach(http_string_parser_ptr& parser) NOEXCEPT;
    static void initialize(http_string_parser_ptr& parser) NOEXCEPT;
    code parse(http_flat_buffer& buffer) NOEXCEPT;

    void read_request() NOEXCEPT;
    void handle_read_request(const code& ec, size_t bytes_read) NOEXCEPT;
    void do_stop(const code& ec) NOEXCEPT;

    request_subscriber subscriber_;
    http_flat_buffer buffer_;
    http_string_parser_ptr parser_{};
};

} // namespace network
} // namespace libbitcoin

#endif
