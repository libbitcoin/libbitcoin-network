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
#ifndef LIBBITCOIN_NETWORK_TOR_LINK_HPP
#define LIBBITCOIN_NETWORK_TOR_LINK_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tor/cell.hpp>
#include <bitcoin/network/tor/certificate.hpp>

namespace libbitcoin {
namespace network {
namespace tor {

/// Initiator link handshake (tor-spec 4) over TLS application data to relay.
/// Sends VERSIONS on construction, receives VERSIONS, CERTS (validated to
/// relay TLS certificate), AUTH_CHALLENGE (ignored) and NETINFO, then sends
/// NETINFO. Initiator does not authenticate. Not thread safe.
class BCT_API link
{
public:
    DELETE_COPY_MOVE(link);

    /// Reason for failed handshake.
    enum class failure : uint8_t
    {
        none,
        malformed,
        unexpected,
        version,
        certificate,
        expired
    };

    /// Relay TLS certificate sha256, relay address, validation time (unix).
    link(const system::hash_digest& certificate,
        const system::ip::address& relay, uint64_t time) NOEXCEPT;

    /// Consume received bytes, false once failed.
    bool receive(const const_byte_span& data) NOEXCEPT;

    /// Bytes to send (caller consumes them).
    system::data_chunk& output() NOEXCEPT;

    /// Bytes received after responder NETINFO (channel cells).
    system::data_chunk& remainder() NOEXCEPT;

    bool is_established() const NOEXCEPT;
    bool is_failed() const NOEXCEPT;
    failure reason() const NOEXCEPT;

    /// Negotiated link protocol version.
    uint16_t version() const NOEXCEPT;

    /// Relay identities, sha1 of der RSA key and Ed25519 key (established).
    const system::short_hash& rsa_identity() const NOEXCEPT;
    const system::ed25519::key& ed25519_identity() const NOEXCEPT;

protected:
    bool fail(failure value) NOEXCEPT;
    bool handle_cell(const cell& value) NOEXCEPT;
    bool handle_versions(const cell& value) NOEXCEPT;
    bool handle_certs(const cell& value) NOEXCEPT;
    bool handle_netinfo(const cell& value) NOEXCEPT;
    void send_netinfo() NOEXCEPT;

private:
    enum class state
    {
        versions,
        certs,
        netinfo,
        established
    };

    // Configuration.
    const system::hash_digest certificate_;
    const system::ip::address relay_;
    const uint64_t time_;

    // State.
    state state_{ state::versions };
    failure failure_{ failure::none };
    uint16_t version_{};
    bool challenged_{};
    system::short_hash rsa_identity_{};
    system::ed25519::key ed25519_identity_{};

    // Buffers.
    system::data_chunk input_{};
    system::data_chunk output_{};
};

} // namespace tor
} // namespace network
} // namespace libbitcoin

#endif
