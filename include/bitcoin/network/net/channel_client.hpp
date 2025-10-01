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
#include <bitcoin/system.hpp>
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
    typedef subscriber<asio::http_request> request_subscriber;
    typedef request_subscriber::handler request_notifier;

    typedef std::shared_ptr<channel_client> ptr;

    /// Subscribe to http_response from peer (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Message, typename Handler = asio::http_request,
        if_same<Message, asio::http_request> = true>
    void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        subscriber_.subscribe(std::forward<Handler>(handler));
    }

    /// Serialize and write http_response to peer (requires strand).
    /// Completion handler is always invoked on the channel strand.
    template <class Message, if_same<Message, asio::http_response> = true>
    void send(const Message& response, result_handler&& complete) NOEXCEPT
    {
        BC_ASSERT_MSG(stranded(), "strand");
        BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
        BC_PUSH_WARNING(NO_UNSAFE_COPY_N)

        if (!response.body().empty() && !response.has_content_length())
        {
            const code ec{ error::oversized_payload };
            LOGF("Serialization failure (http_response), " << ec.message());
            complete(ec);
            return;
        }

        using namespace system;
        using namespace boost::beast;
        const auto& body = response.body();
        const auto body_size = body.size();
        const auto data = std::make_shared<data_chunk>();
        size_t head_size{};

        const auto head_writer = [&data, &head_size, body_size](
            error_code& out, const auto& buffers) NOEXCEPT
        {
            using namespace boost::asio;
            head_size = buffer_size(buffers);
            data->resize(ceilinged_add(head_size, body_size));
            buffer_copy(buffer(data->data(), head_size), buffers);
            out = error::success;
        };

        // Serialize headers to buffer.
        error_code ec{};
        asio::http_serializer writer{ response };
        writer.split(true);
        writer.next(ec, head_writer);
        writer.consume(head_size);

        // Above assumes all headers are passed one iteration.
        if (!ec && !writer.is_header_done())
            ec = http::error::header_limit;

        if (ec)
        {
            LOGF("Serialization failure (http_response), " << ec.message());
            complete(ec);
            return;
        }

        // Copy body directly into data_chunk after headers.
        const auto from = pointer_cast<const uint8_t>(body.data());
        std::copy_n(from, body_size, std::next(data->data(), head_size));
        write(data, std::move(complete));

        BC_POP_WARNING()
        BC_POP_WARNING()
    }

    /// Construct client channel to encapsulate and communicate on the socket.
    channel_client(const logger& log, const network::socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero) NOEXCEPT;

    /// Idempotent, may be called multiple times.
    void stop(const code& ec) NOEXCEPT override;

    /// Resume reading from the socket (requires strand).
    void resume() NOEXCEPT override;

private:
    using http_parser_ptr = std::unique_ptr<asio::http_parser>;

    // Parser utilities.
    static asio::http_request detach(http_parser_ptr& parser) NOEXCEPT;
    static void initialize(http_parser_ptr& parser) NOEXCEPT;
    code parse(asio::http_buffer& buffer) NOEXCEPT;

    void read_request() NOEXCEPT;
    void handle_read_request(const code& ec, size_t bytes_read) NOEXCEPT;
    void do_stop(const code& ec) NOEXCEPT;

    request_subscriber subscriber_;
    asio::http_buffer buffer_;
    http_parser_ptr parser_{};
};

} // namespace network
} // namespace libbitcoin

#endif
