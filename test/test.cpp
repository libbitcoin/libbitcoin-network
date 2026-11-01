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
#include "test.hpp"

#include <filesystem>

// copied from libbitcoin-system-test

namespace std {

std::ostream& operator<<(std::ostream& stream,
    const system::data_slice& slice) NOEXCEPT
{
    // Avoid serialize() here for its own test benefit.
    // stream << serialize(slice);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    stream << encode_base16(slice);
    BC_POP_WARNING()
    return stream;
}

} // namespace std

namespace test {

// CERT_WRITE_TEMP_DIR defined by build should match as "tests/".
const std::string directory = "tests";

bool clear(const std::filesystem::path& file_directory) NOEXCEPT
{
    // remove_all returns count removed, and error code if fails.
    // create_directories returns true if path exists or created.
    // used for setup, with no expectations of file/directory existence.
    const auto path = system::extended_path(file_directory);
    std::error_code ec;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::filesystem::remove_all(path, ec);
    return !ec && std::filesystem::create_directories(path, ec);
    BC_POP_WARNING()
}

bool create(const std::filesystem::path& file_path) NOEXCEPT
{
    // Creates and returns true if file already existed (and no error).
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::ofstream file(system::extended_path(file_path));
    return file.good();
    BC_POP_WARNING()
}

bool exists(const std::filesystem::path& file_path) NOEXCEPT
{
    // Returns true only if file existed.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::ifstream file(system::extended_path(file_path));
    return file.good();
    BC_POP_WARNING()
}

bool remove(const std::filesystem::path& file_path) NOEXCEPT
{
    // Deletes and returns false if file did not exist (or error).
    std::error_code ec;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return std::filesystem::remove(system::extended_path(file_path), ec);
    BC_POP_WARNING()
}

network::settings network_settings() NOEXCEPT
{
    using namespace messages::peer;
    network::settings out{ system::chain::selection::none };
    out.network = system::chain::selection::mainnet;
    out.identifier = 3652501241;
    out.protocol_maximum = level::maximum_protocol;
    out.protocol_minimum = level::minimum_protocol;
    out.enable_address = false;
    out.enable_address_v2 = true;
    out.enable_witness_tx = false;
    out.enable_compact = false;
    out.enable_not_found = false;
    out.enable_relay = false;
    out.maximum_skew_minutes = 120;

    out.outbound.connections = 10;
    out.outbound.connect_batch_size = 5;
    out.outbound.host_pool_capacity = 0;
    out.outbound.seeds.clear();
    out.outbound.seeds.emplace_back("mainnet1.libbitcoin.net", 8333_u16);
    out.outbound.seeds.emplace_back("mainnet2.libbitcoin.net", 8333_u16);
    out.outbound.seeds.emplace_back("mainnet3.libbitcoin.net", 8333_u16);
    out.outbound.seeds.emplace_back("mainnet4.libbitcoin.net", 8333_u16);

    out.inbound.connections = 0;
    out.inbound.binds.clear();
    out.inbound.binds.emplace_back(asio::address{}, 8333_u16);
    return out;
}

} // namespace test
