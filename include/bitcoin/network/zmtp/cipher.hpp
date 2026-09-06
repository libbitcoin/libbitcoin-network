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
#ifndef LIBBITCOIN_NETWORK_ZMTP_CIPHER_HPP
#define LIBBITCOIN_NETWORK_ZMTP_CIPHER_HPP

#include <optional>
#include <span>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace zmtp {

/// The CurveZMQ (rfc.zeromq.org/spec/26) mechanism state of one connection.
/// Boxes are NaCl crypto_box (x25519, hsalsa20, xsalsa20-poly1305). The
/// handshake commands (HELLO, WELCOME, INITIATE, READY) and MESSAGE are
/// produced and consumed as ZMTP command bodies (name-length, name, fields),
/// framing being left to the stream. Not thread safe.
class BCT_API cipher final
{
public:
    DELETE_COPY_MOVE(cipher);

    typedef system::x25519::key key;
    typedef std::span<const uint8_t> span;

    /// Constants.
    static constexpr size_t key_size = system::x25519::key_size;
    static constexpr size_t tag_size = system::xsalsa20_poly1305::expansion;
    static constexpr size_t nonce_size = system::salsa20::extended_nonce_size;
    static constexpr size_t short_nonce_size = sizeof(uint64_t);
    static constexpr size_t long_nonce_size = nonce_size - short_nonce_size;
    static constexpr size_t cookie_size = long_nonce_size + tag_size +
        2 * key_size;

    /// Command body sizes (the name prefix is included).
    static constexpr size_t hello_size = 200;
    static constexpr size_t welcome_size = 168;
    static constexpr size_t initiate_minimum = 257;
    static constexpr size_t ready_minimum = 30;
    static constexpr size_t message_minimum = 33;

    /// Flags within a MESSAGE box (distinct from frame flags).
    static constexpr uint8_t payload_more = 0x01;
    static constexpr uint8_t payload_command = 0x02;


    /// Derive the public key of a secret (false if the secret is invalid).
    static bool to_public(key& public_key, const key& secret) NOEXCEPT;

    /// Server: own long-term keypair.
    cipher(const key& secret, const key& public_key) NOEXCEPT;

    /// Client: own long-term keypair and the server public key.
    cipher(const key& secret, const key& public_key,
        const key& server) NOEXCEPT;

    /// Own long-term public key.
    const key& public_key() const NOEXCEPT;

    /// Peer long-term public key (server: valid after ready).
    const key& peer_key() const NOEXCEPT;

    /// Handshake (client: hello, initiate, complete; server: welcome, ready).
    /// Each returns false on any protocol violation, after which the
    /// connection must be dropped. The out parameters are command bodies.
    bool hello(system::data_chunk& out) NOEXCEPT;
    bool welcome(system::data_chunk& out, const span& hello) NOEXCEPT;
    bool initiate(system::data_chunk& out, const span& welcome,
        const span& metadata) NOEXCEPT;
    bool ready(system::data_chunk& out, system::data_chunk& peer_metadata,
        const span& initiate, const span& metadata) NOEXCEPT;
    bool complete(system::data_chunk& peer_metadata,
        const span& ready) NOEXCEPT;

    /// Box one frame into a MESSAGE command body (after handshake).
    bool encode(system::data_chunk& out, uint8_t flags,
        const span& body) NOEXCEPT;

    /// Unbox a MESSAGE command body into frame flags and body (after
    /// handshake). False if the nonce is not increasing or the tag fails.
    bool decode(uint8_t& flags, system::data_chunk& body,
        const span& message) NOEXCEPT;

private:
    typedef system::xsalsa20_poly1305 box;
    typedef system::salsa20::extended_nonce nonce;
    typedef system::data_array<long_nonce_size> long_nonce;

    static bool derive(key& out, const key& secret,
        const key& public_key) NOEXCEPT;
    static nonce make_nonce(const std::string& prefix,
        uint64_t value) NOEXCEPT;
    static nonce make_nonce(const std::string& prefix,
        const long_nonce& value) NOEXCEPT;
    static void seal(system::data_chunk& out, box& sealer, const nonce& value,
        const span& plain) NOEXCEPT;
    static bool open(system::data_chunk& out, box& opener, const nonce& value,
        const span& cipher) NOEXCEPT;
    static bool named(const span& body, const std::string& name) NOEXCEPT;
    static void name(system::data_chunk& out,
        const std::string& name) NOEXCEPT;

    bool accept(uint64_t nonce) NOEXCEPT;

    // Long-term keys.
    const bool server_;
    key secret_;
    key public_;
    key peer_;

    // Transient (per connection) keys and cookie key.
    key transient_secret_{};
    key transient_public_{};
    key peer_transient_{};
    key cookie_key_{};

    // Nonces are strictly increasing in each direction.
    uint64_t nonce_{ 1 };
    uint64_t peer_nonce_{};

    // Precomputed boxes.
    std::optional<box> hello_box_{};
    std::optional<box> vouch_box_{};
    std::optional<box> session_box_{};
};

} // namespace zmtp
} // namespace network
} // namespace libbitcoin

#endif
