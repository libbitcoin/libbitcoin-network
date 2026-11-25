/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_SV2_HPP
#define LIBBITCOIN_NETWORK_CHANNELS_CHANNEL_SV2_HPP

#include <bitcoin/network/channels/channel_http.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/interfaces/interfaces.hpp>
#include <bitcoin/network/log/log.hpp>
#include <bitcoin/network/memory.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/network/messages/sv2/registry.hpp>

namespace libbitcoin {
namespace network {

/// Full duplex stratum v2 channel, served over ws/s (by upgrade), one frame
/// to each binary message.
class BCT_API channel_sv2
  : public channel_http
{
public:
    typedef std::shared_ptr<channel_sv2> ptr;
    using interface = rpc::interface::sv2::dispatch;
    using dispatcher = rpc::dispatcher<interface>;
    using channel_http::channel_http;

    /// Subscribe to messages from client (requires strand).
    /// Event handler always invoked on channel strand.
    template <class Message>
    inline void subscribe(auto&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());
        using signature = interface::signature<Message>;
        dispatcher_.subscribe(std::forward<signature>(handler));
    }

    /// Write message to client (requires strand).
    /// Message translated to wire by body (transport framed).
    /// Completion handler always invoked on channel strand.
    template <class Message>
    inline void send(const Message& message, result_handler&& handler) NOEXCEPT
    {
        BC_ASSERT(stranded());

        messages::sv2::frame out{};
        out.message = rpc::any_t{ system::to_shared(message) };
        out.index = messages::sv2::registry::index_of<Message>();
        out.size = message.size();

        http::response response{ http::status::ok, http::version_1_1 };
        response.body() = std::move(out);
        notify(std::move(response), std::move(handler));
    }

    /// The handshake is complete, set only by the handshake (requires strand).
    bool handshaked() const NOEXCEPT;
    void set_handshaked() NOEXCEPT;

protected:
    /// Stranded handler invoked from stop().
    void stopping(const code& ec) NOEXCEPT override;

    /// The preselected reader body, stamped with parse context.
    http::body::value_type default_body() const NOEXCEPT override;

    /// Dispatch message to subscribers by message type.
    void dispatch(const http::request_ptr& request) NOEXCEPT override;

    void handle_receive(const code& ec, size_t bytes,
        const http::request_ptr& request) NOEXCEPT override;

private:
    void handle_refused(const code& ec) NOEXCEPT;

    // These are protected by strand.
    dispatcher dispatcher_{};
    bool handshaked_{};
};

} // namespace network
} // namespace libbitcoin

#endif
