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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_RPC_HPP
#define LIBBITCOIN_NETWORK_PROTOCOL_RPC_HPP

#include <memory>
#include <utility>
#include <bitcoin/network/channels/channels.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/protocols/protocol.hpp>

namespace libbitcoin {
namespace network {
    
template <typename Interface>
class protocol_rpc
 : public protocol
{
public:
    typedef std::shared_ptr<protocol> ptr;
    using protocol_t = protocol_rpc<Interface>;
    using channel_t = channel_rpc<Interface>;
    using options_t = channel_t::options_t;

protected:
    inline protocol_rpc(const session::ptr& session,
        const channel::ptr& channel, const options_t&) NOEXCEPT
      : protocol(session, channel),
        channel_(std::dynamic_pointer_cast<channel_t>(channel))
    {
    }

    DECLARE_SUBSCRIBE_CHANNEL()

    template <class Derived, typename Method, typename... Args>
    inline void send(network::rpc::response_t&& message, size_t size_hint,
        Method&& method, Args&&... args) NOEXCEPT
    {
        channel_->send(std::move(message), size_hint,
            std::bind(std::forward<Method>(method),
                shared_from_base<Derived>(), std::forward<Args>(args)...));
    }

    // TODO: capture and correlate version/id.
    inline void send_result(network::rpc::value_t&& value,
        size_t size_hint) NOEXCEPT
    {
        using namespace network::rpc;
        using namespace std::placeholders;
        send<protocol_t>(
            {
                .jsonrpc = version::v2,
                .id = 42,
                ////.error = {},
                .result = std::move(value)
            },
            size_hint, &protocol_t::handle_complete, _1, error::success);
    }

    // TODO: capture and correlate version/id.
    inline void send_error(const code& reason) NOEXCEPT
    {
        using namespace network::rpc;
        using namespace std::placeholders;
        const auto size_hint = two * reason.message().size();
        send<protocol_t>(
            {
                .jsonrpc = version::v2,
                .id = 42,
                .error = result_t
                {
                    .code = reason.value(),
                    .message = reason.message()
                }
                ////.result = {}
            },
            size_hint, &protocol_t::handle_complete, _1, reason);
    }

    inline void handle_complete(const code& ec, const code& reason) NOEXCEPT
    {
        BC_ASSERT(stranded());

        if (stopped(ec))
            return;

        if (reason)
        {
            stop(reason);
            return;
        }

        // Continue read loop.
        channel_->receive();
    }

private:
    // This is mostly thread safe, and used in a thread safe manner.
    // pause/resume/paused/attach not invoked, setters limited to handshake.
    const channel_t::ptr channel_;
};

} // namespace network
} // namespace libbitcoin

#endif
