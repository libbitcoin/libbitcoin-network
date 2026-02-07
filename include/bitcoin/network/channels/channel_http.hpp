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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_HTTP_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_HTTP_HPP

#include <memory>
#include <utility>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/interfaces/interfaces.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>

namespace libbitcoin {
namespace network {

/// Half-duplex reading/writing of http-request/response.
class BCT_API channel_http
  : public channel
{
public:
    typedef std::shared_ptr<channel_http> ptr;
    using options_t = settings_t::http_server;
    using interface = rpc::interface::http;
    using dispatcher = rpc::dispatcher<interface>;

    /// Subscribe to message from client (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Request>
    inline void subscribe(auto&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());
        using signature = interface::signature<Request>;
        dispatcher_.subscribe(std::forward<signature>(handler));
    }

    /// Construct http channel to encapsulate and communicate on the socket.
    inline channel_http(const logger& log, const socket::ptr& socket,
        uint64_t identifier, const settings_t& settings,
        const options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options),
        options_(options),
        response_buffer_(system::to_shared<http::flat_buffer>()),
        request_buffer_(options.minimum_buffer)
    {
    }

    /// Serialize and write http message to client (requires strand).
    /// Completion handler is always invoked on the channel strand.
    void send(http::response&& response,
        result_handler&& handler) NOEXCEPT;

    /// Resume reading from the socket (requires strand).
    void resume() NOEXCEPT override;

    /// Must call after successful message handling if no stop.
    virtual void receive() NOEXCEPT;

protected:
    /// Stranded handler invoked from stop().
    void stopping(const code& ec) NOEXCEPT override;

    /// Read request buffer (requires strand).
    virtual http::flat_buffer& request_buffer() NOEXCEPT;

    /// Determine if http basic authorization is satisfied if enabled.
    virtual bool unauthorized(const http::request& request) NOEXCEPT;

    /// Dispatch request to subscribers by verb type.
    virtual void dispatch(const http::request_cptr& request) NOEXCEPT;

    /// Size and assign response_buffer_ if value type is json or json-rpc.
    virtual void assign_json_buffer(http::response& response) NOEXCEPT;

    /// Handlers.
    virtual void handle_receive(const code& ec, size_t bytes,
        const http::request_cptr& request) NOEXCEPT;
    virtual void handle_send(const code& ec, size_t bytes,
        const http::response_cptr& response,
        const result_handler& handler) NOEXCEPT;

private:
    void handle_unauthorized(const code& ec) NOEXCEPT;
    void log_message(const http::request& request,
        size_t bytes) const NOEXCEPT;
    void log_message(const http::response& response,
        size_t bytes) const NOEXCEPT;

    // This is thread safe.
    const options_t& options_;

    // These are protected by strand.
    http::flat_buffer_ptr response_buffer_;
    http::flat_buffer request_buffer_;
    dispatcher dispatcher_{};
    bool reading_{};
};

} // namespace network
} // namespace libbitcoin

#endif
