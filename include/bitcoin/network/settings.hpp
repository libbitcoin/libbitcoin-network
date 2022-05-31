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
#ifndef LIBBITCOIN_NETWORK_SETTINGS_HPP
#define LIBBITCOIN_NETWORK_SETTINGS_HPP

#include <cstddef>
#include <cstdint>
#include <boost/asio.hpp>
////#include <boost/filesystem.hpp>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/config.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// Common database configuration settings, properties not thread safe.
class BCT_API settings
{
public:
    settings() noexcept;
    settings(system::chain::selection context)noexcept ;

    /// Properties.
    uint32_t threads;
    uint32_t protocol_maximum;
    uint32_t protocol_minimum;
    uint64_t services_maximum;
    uint64_t services_minimum;
    uint64_t invalid_services;
    bool enable_reject;
    bool relay_transactions;
    bool validate_checksum;
    uint32_t identifier;
    uint16_t inbound_port;
    uint32_t inbound_connections;
    uint32_t outbound_connections;
    uint32_t connect_batch_size;
    uint32_t connect_timeout_seconds;
    uint32_t channel_handshake_seconds;
    uint32_t channel_germination_seconds;
    uint32_t channel_heartbeat_minutes;
    uint32_t channel_inactivity_minutes;
    uint32_t channel_expiration_minutes;
    uint32_t host_pool_capacity;
    boost::filesystem::path hosts_file;
    config::authority self;
    config::authorities blacklists;
    config::endpoints peers;
    config::endpoints seeds;

    // [log]
    ////boost::filesystem::path debug_file;
    ////boost::filesystem::path error_file;
    ////boost::filesystem::path archive_directory;
    ////size_t rotation_size;
    ////size_t minimum_free_space;
    ////size_t maximum_archive_size;
    ////size_t maximum_archive_files;
    ////config::authority statistics_server;
    bool verbose;

    /// Helpers.
    duration connect_timeout() const noexcept;
    duration channel_handshake() const noexcept;
    duration channel_heartbeat() const noexcept;
    duration channel_inactivity() const noexcept;
    duration channel_expiration() const noexcept;
    duration channel_germination() const noexcept;
};

} // namespace network
} // namespace libbitcoin

#endif
