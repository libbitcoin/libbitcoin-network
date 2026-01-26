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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_RPC_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_RPC_HPP

#include <memory>
#include <bitcoin/network/channels/channel.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/net/net.hpp>

namespace libbitcoin {
namespace network {

/// Read rpc-request and send rpc-response, dispatch to Interface.
template <typename Interface>
class channel_rpc
  : public channel
{
public:
    typedef std::shared_ptr<channel_rpc> ptr;
    using options_t = network::settings::tls_server;
    using dispatcher = rpc::dispatcher<Interface>;

    /// Subscribe to request from client (requires strand).
    /// Event handler is always invoked on the channel strand.
    template <class Unused, class Handler>
    inline void subscribe(Handler&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());
        dispatcher_.subscribe(std::forward<Handler>(handler));
    }

    /// Construct rpc channel to encapsulate and communicate on the socket.
    inline channel_rpc(const logger& log, const socket::ptr& socket,
        uint64_t identifier, const settings_t& settings,
        const options_t& options) NOEXCEPT
      : channel(log, socket, identifier, settings, options),
        response_buffer_(system::to_shared<http::flat_buffer>()),
        request_buffer_(options.minimum_buffer)
    {
    }

    /// Senders, rpc version and identity added to responses (requires strand).
    inline void send_code(const code& ec) NOEXCEPT;
    inline void send_error(rpc::result_t&& error) NOEXCEPT;
    inline void send_result(rpc::value_t&& result, size_t size_hint) NOEXCEPT;

    /// Senders with completion handlers (requires strand).
    inline void send_code(const code& ec, result_handler&& handler) NOEXCEPT;
    inline void send_error(rpc::result_t&& error,
        result_handler&& handler) NOEXCEPT;
    inline void send_result(rpc::value_t&& result, size_t size_hint,
        result_handler&& handler) NOEXCEPT;

    /// Resume reading from the socket (requires strand).
    inline void resume() NOEXCEPT override;

protected:
    /// Serialize and write response to client (requires strand).
    /// Completion handler is always invoked on the channel strand.
    inline void send(rpc::response_t&& message, size_t size_hint,
        result_handler&& handler) NOEXCEPT;

    /// Stranded handler invoked from stop().
    inline void stopping(const code& ec) NOEXCEPT override;

    /// Read request buffer (requires strand).
    virtual inline http::flat_buffer& request_buffer() NOEXCEPT;

    /// Override to dispatch request to subscribers by requested method.
    virtual inline void dispatch(const rpc::request_cptr& request) NOEXCEPT;

    /// Size and assign response_buffer_ (value type is json-rpc::json).
    virtual inline rpc::response_ptr assign_message(rpc::response_t&& message,
        size_t size_hint) NOEXCEPT;

    /// Must call after successful message handling if no stop.
    virtual inline void receive() NOEXCEPT;

    /// Handle incoming messages.
    virtual inline void handle_receive(const code& ec, size_t bytes,
        const rpc::request_cptr& request) NOEXCEPT;

    /// Handle send completion, invokes receive().
    virtual inline void handle_send(const code& ec, size_t bytes,
        const rpc::response_cptr& response,
        const result_handler& handler) NOEXCEPT;

    /// Default noop completion handler.
    virtual inline void complete(const code&) NOEXCEPT {};

private:
    // These are protected by strand.
    rpc::version version_;
    rpc::id_option identity_;
    http::flat_buffer_ptr response_buffer_;
    http::flat_buffer request_buffer_;
    dispatcher dispatcher_{};
    bool reading_{};
};

} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <typename Interface>
#define CLASS channel_rpc<Interface>

#include <bitcoin/network/impl/channels/channel_rpc.ipp>

#undef CLASS
#undef TEMPLATE

#endif
