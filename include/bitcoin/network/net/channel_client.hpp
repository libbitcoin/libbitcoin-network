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
    typedef std::shared_ptr<channel_client> ptr;

    channel_client(const logger& log, const socket::ptr& socket,
        const network::settings& settings, uint64_t identifier=zero) NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#endif
