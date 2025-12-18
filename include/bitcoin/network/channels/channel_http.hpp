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
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/interface/interface.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/rpc/rpc.hpp>
#include <bitcoin/network/net/net.hpp>

namespace libbitcoin {
namespace network {

/// Half-duplex reading of http-request and sending of http-response.
class BCT_API channel_http
  : public channel
{
public:
    typedef std::shared_ptr<channel_http> ptr;
    using options_t = settings_t::http_server;
    using interface = rpc::interface::http;
    using dispatcher = rpc::dispatcher<interface>;

    /// Subscribe to request from peer (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Request>
    inline void subscribe(auto&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());
        using signature = interface::signature<Request>;
        dispatcher_.subscribe(std::forward<signature>(handler));
    }

    // TODO: network.minimum_buffer is being overloaded here.
    /// response_buffer_ is initialized to default size, see set_buffer().
    /// Construct client channel to encapsulate and communicate on the socket.
    inline channel_http(const logger& log, const socket::ptr& socket,
        uint64_t identifier, const settings_t& settings,
        const options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options),
        response_buffer_(system::to_shared<http::flat_buffer>()),
        request_buffer_(settings.minimum_buffer)
    {
    }

    /// Resume reading from the socket (requires strand).
    void resume() NOEXCEPT override;

    /// Must call after successful message handling if no stop.
    virtual void receive() NOEXCEPT;

    /// Serialize and write http response to peer (requires strand).
    /// Completion handler is always invoked on the channel strand.
    virtual void send(http::response&& response,
        result_handler&& handler) NOEXCEPT;

protected:
    /// Stranded handler invoked from stop().
    void stopping(const code& ec) NOEXCEPT override;

    /// Read request buffer (not thread safe).
    virtual http::flat_buffer& request_buffer() NOEXCEPT;

    /// Dispatch request to subscribers by verb type.
    virtual void dispatch(const http::request_cptr& request) NOEXCEPT;

    virtual void size_json_buffer(http::response& response) NOEXCEPT;

    virtual void handle_receive(const code& ec, size_t bytes,
        const http::request_cptr& request) NOEXCEPT;
    virtual void handle_send(const code& ec, size_t bytes, http::response_ptr&,
        const result_handler& handler) NOEXCEPT;

private:
    void log_message(const http::request& request) const NOEXCEPT;
    void log_message(const http::response& response) const NOEXCEPT;

    // These are protected by strand.
    http::flat_buffer_ptr response_buffer_;
    http::flat_buffer request_buffer_;
    dispatcher dispatcher_{};
    bool reading_{};
};

} // namespace network
} // namespace libbitcoin

#endif
