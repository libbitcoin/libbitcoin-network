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
#ifndef LIBBITCOIN_NETWORK_PROTOCOL_RPC_IPP
#define LIBBITCOIN_NETWORK_PROTOCOL_RPC_IPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {

TEMPLATE
inline void CLASS::send_code(const code& ec) NOEXCEPT
{
    channel_->send_code(ec);
}

TEMPLATE
inline void CLASS::send_error(rpc::result_t&& error) NOEXCEPT
{
    channel_->send_error(std::move(error));
}

TEMPLATE
inline void CLASS::send_result(rpc::value_t&& result,
    size_t size_hint) NOEXCEPT
{
    channel_->send_result(std::move(result), size_hint);
}

} // namespace network
} // namespace libbitcoin

#endif
