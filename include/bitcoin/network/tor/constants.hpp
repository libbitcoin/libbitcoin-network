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
#ifndef LIBBITCOIN_NETWORK_TOR_CONSTANTS_HPP
#define LIBBITCOIN_NETWORK_TOR_CONSTANTS_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace tor {

/// Tor wire values (tor-spec, cert-spec).

/// Link protocol versions (4: four byte circuit ids, 5: link padding).
constexpr std::array<uint16_t, 2> link_versions{ 4, 5 };

/// Cell sizes (tor-spec 3), VERSIONS cell has narrow circuit ids.
constexpr size_t narrow_circuit_size = 2;
constexpr size_t circuit_size = 4;
constexpr size_t payload_size = 509;

/// Channel commands (tor-spec 3).
namespace command
{
    constexpr uint8_t padding = 0;
    constexpr uint8_t create = 1;
    constexpr uint8_t created = 2;
    constexpr uint8_t relay = 3;
    constexpr uint8_t destroy = 4;
    constexpr uint8_t create_fast = 5;
    constexpr uint8_t created_fast = 6;
    constexpr uint8_t versions = 7;
    constexpr uint8_t netinfo = 8;
    constexpr uint8_t relay_early = 9;
    constexpr uint8_t create2 = 10;
    constexpr uint8_t created2 = 11;
    constexpr uint8_t padding_negotiate = 12;
    constexpr uint8_t vpadding = 128;
    constexpr uint8_t certs = 129;
    constexpr uint8_t auth_challenge = 130;
    constexpr uint8_t authenticate = 131;
    constexpr uint8_t authorize = 132;
}

/// Certificate types of CERTS cell and Tor certificates (tor-spec 4.2).
namespace certificate_type
{
    constexpr uint8_t link_x509 = 1;
    constexpr uint8_t rsa_identity = 2;
    constexpr uint8_t authenticate_x509 = 3;
    constexpr uint8_t identity_signing = 4;
    constexpr uint8_t signing_link = 5;
    constexpr uint8_t signing_authenticate = 6;
    constexpr uint8_t rsa_crosscert = 7;
}

/// Certified key types (cert-spec 2.1).
namespace key_type
{
    constexpr uint8_t ed25519 = 1;
    constexpr uint8_t sha256_rsa = 2;
    constexpr uint8_t sha256_x509 = 3;
}

/// Certificate version, extension and flag (cert-spec 2.1, 2.2).
constexpr uint8_t certificate_version = 1;
constexpr uint8_t signed_with_ed25519 = 4;
constexpr uint8_t affects_validation = 1;

/// NETINFO address types (tor-spec 4.5).
constexpr uint8_t address_ipv4 = 4;
constexpr uint8_t address_ipv6 = 6;

} // namespace tor
} // namespace network
} // namespace libbitcoin

#endif
