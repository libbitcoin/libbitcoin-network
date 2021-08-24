/**
 * Copyright (c) 2011-2019 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_LOG_STATSD_SINK_HPP
#define LIBBITCOIN_NETWORK_LOG_STATSD_SINK_HPP

#include <cstdint>
#include <bitcoin/network/concurrent/asio.hpp>
#include <bitcoin/network/concurrent/threadpool.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/log/rotable_file.hpp>

namespace libbitcoin {
namespace network {
namespace log {

void initialize_statsd(const rotable_file& file);

void initialize_statsd(threadpool& pool, const asio::ipv6& ip,
    uint16_t port);

} // namespace log
} // namespace network
} // namespace libbitcoin

#endif
