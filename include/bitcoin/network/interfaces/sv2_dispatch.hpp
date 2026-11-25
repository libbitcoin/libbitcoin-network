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
#ifndef LIBBITCOIN_NETWORK_INTERFACES_SV2_DISPATCH_HPP
#define LIBBITCOIN_NETWORK_INTERFACES_SV2_DISPATCH_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct sv2_dispatch
{
    /// Unsubscriber requires bool handlers, injects `code` parameter.
    template <typename... Args>
    using subscriber = network::unsubscriber<Args...>;

    /// dispatcher.subscribe(std::forward<signature>(handler));
    template <class Message>
    using signature = std::function<bool(const code&,
        const typename Message::cptr&)>;

    /// Messages exchanged after the handshake completes.
    static constexpr std::tuple messages
    {
        method<"channel_endpoint_changed", messages::sv2::channel_endpoint_changed::cptr>{},
        method<"reconnect", messages::sv2::reconnect::cptr>{},
        method<"open_standard_mining_channel", messages::sv2::open_standard_mining_channel::cptr>{},
        method<"open_standard_mining_channel_success", messages::sv2::open_standard_mining_channel_success::cptr>{},
        method<"open_mining_channel_error", messages::sv2::open_mining_channel_error::cptr>{},
        method<"open_extended_mining_channel", messages::sv2::open_extended_mining_channel::cptr>{},
        method<"open_extended_mining_channel_success", messages::sv2::open_extended_mining_channel_success::cptr>{},
        method<"new_mining_job", messages::sv2::new_mining_job::cptr>{},
        method<"update_channel", messages::sv2::update_channel::cptr>{},
        method<"update_channel_error", messages::sv2::update_channel_error::cptr>{},
        method<"close_channel", messages::sv2::close_channel::cptr>{},
        method<"set_extranonce_prefix", messages::sv2::set_extranonce_prefix::cptr>{},
        method<"submit_shares_standard", messages::sv2::submit_shares_standard::cptr>{},
        method<"submit_shares_extended", messages::sv2::submit_shares_extended::cptr>{},
        method<"submit_shares_success", messages::sv2::submit_shares_success::cptr>{},
        method<"submit_shares_error", messages::sv2::submit_shares_error::cptr>{},
        method<"new_extended_mining_job", messages::sv2::new_extended_mining_job::cptr>{},
        method<"set_new_prev_hash", messages::sv2::set_new_prev_hash::cptr>{},
        method<"set_target", messages::sv2::set_target::cptr>{},
        method<"set_custom_mining_job", messages::sv2::set_custom_mining_job::cptr>{},
        method<"set_custom_mining_job_success", messages::sv2::set_custom_mining_job_success::cptr>{},
        method<"set_custom_mining_job_error", messages::sv2::set_custom_mining_job_error::cptr>{},
        method<"set_group_channel", messages::sv2::set_group_channel::cptr>{}
    };

    /// Handshake methods (exchanged before the handshake completes).
    static constexpr std::tuple handshake
    {
        method<"setup_connection", messages::sv2::setup_connection::cptr>{},
        method<"setup_connection_success", messages::sv2::setup_connection_success::cptr>{},
        method<"setup_connection_error", messages::sv2::setup_connection_error::cptr>{}
    };

    /// The channel dispatches on the union (the wire carries both).
    static constexpr auto methods = std::tuple_cat(handshake, messages);
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
