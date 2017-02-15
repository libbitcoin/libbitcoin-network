/**
 * Copyright (c) 2011-2017 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_SETTINGS_HPP
#define LIBBITCOIN_NETWORK_SETTINGS_HPP

#include <cstddef>
#include <cstdint>
#include <bitcoin/bitcoin.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Common database configuration settings, properties not thread safe.
class BCT_API settings
{
public:
    settings();
    settings(config::settings context);

    /// Properties.
    uint32_t threads;
    uint32_t protocol_maximum;
    uint32_t protocol_minimum;
    uint64_t services;
    bool relay_transactions;
    bool validate_checksum;
    uint32_t identifier;
    uint16_t inbound_port;
    uint32_t inbound_connections;
    uint32_t outbound_connections;
    uint32_t manual_attempt_limit;
    uint32_t connect_batch_size;
    uint32_t connect_timeout_seconds;
    uint32_t channel_handshake_seconds;
    uint32_t channel_heartbeat_minutes;
    uint32_t channel_inactivity_minutes;
    uint32_t channel_expiration_minutes;
    uint32_t channel_germination_seconds;
    uint32_t host_pool_capacity;
    boost::filesystem::path hosts_file;
    config::authority self;
    config::authority::list blacklists;
    config::endpoint::list peers;
    config::endpoint::list seeds;

    // [log]
    boost::filesystem::path debug_file;
    boost::filesystem::path error_file;
    boost::filesystem::path archive_directory;
    size_t rotation_size;
    size_t minimum_free_space;
    size_t maximum_archive_size;
    size_t maximum_archive_files;
    config::authority statistics_server;
    bool verbose;

    /// Helpers.
    asio::duration connect_timeout() const;
    asio::duration channel_handshake() const;
    asio::duration channel_heartbeat() const;
    asio::duration channel_inactivity() const;
    asio::duration channel_expiration() const;
    asio::duration channel_germination() const;
};

} // namespace network
} // namespace libbitcoin

#endif
