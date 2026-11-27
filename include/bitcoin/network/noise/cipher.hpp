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
#ifndef LIBBITCOIN_NETWORK_NOISE_CIPHER_HPP
#define LIBBITCOIN_NETWORK_NOISE_CIPHER_HPP

#include <optional>
#include <span>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace noise {

/// Stratum v2 Noise NX (Noise_NX_Secp256k1+EllSwift_ChaChaPoly_SHA256)
/// handshake and transport state of one connection. The initiator sends act1,
/// the responder answers with act2 (its static key and certificate), and both
/// then derive directional transport ciphers. Transport frames are sv2 frames
/// (heading then payload), the heading and each payload chunk sealed as
/// separate blocks. Not thread safe.
class BCT_API cipher final
{
public:
    DELETE_COPY_MOVE(cipher);

    /// Constants.
    static constexpr size_t key_size = system::ec_ellswift_size;
    static constexpr size_t tag_size = system::chacha20_poly1305::expansion;
    static constexpr size_t certificate_size = 74;
    static constexpr size_t act1_size = key_size;
    static constexpr size_t act2_size = key_size + key_size + tag_size +
        certificate_size + tag_size;
    static constexpr size_t heading_size = 6;
    static constexpr size_t sealed_heading_size = heading_size + tag_size;
    static constexpr size_t chunk_size = 65535 - tag_size;

    typedef system::ec_ellswift key;
    typedef system::data_array<certificate_size> certificate;
    typedef system::data_array<act1_size> act1;
    typedef system::data_array<act2_size> act2;
    typedef std::span<const uint8_t> span;

    /// Sealed size of a payload of the given plaintext size.
    static constexpr size_t sealed_size(size_t payload) NOEXCEPT
    {
        const auto remainder = payload % chunk_size;
        return (payload / chunk_size) * (chunk_size + tag_size) +
            (is_zero(remainder) ? zero : remainder + tag_size);
    }

    /// Initiator: generate an ephemeral keypair.
    cipher() NOEXCEPT;

    /// Initiator: given ephemeral keypair (deterministic, for test vectors).
    cipher(const system::ec_secret& secret, const key& public_key) NOEXCEPT;

    /// Responder: static keypair and certificate, generates ephemeral keypair.
    cipher(const system::ec_secret& static_secret, const key& static_key,
        const certificate& proof) NOEXCEPT;

    /// Responder: given ephemeral keypair (deterministic, for test vectors).
    cipher(const system::ec_secret& secret, const key& public_key,
        const system::ec_secret& static_secret, const key& static_key,
        const certificate& proof) NOEXCEPT;

    /// Own ephemeral public key (ElligatorSwift encoded).
    const key& public_key() const NOEXCEPT;

    /// Handshake hash (final after respond or complete).
    const system::hash_digest& handshake_hash() const NOEXCEPT;

    /// Initiator: produce act1.
    void initiate(act1& out) NOEXCEPT;

    /// Responder: consume act1 and produce act2, deriving transport ciphers.
    bool respond(act2& out, const act1& in) NOEXCEPT;

    /// Initiator: consume act2, obtaining the responder static key and its
    /// (unverified) certificate, and deriving transport ciphers.
    bool complete(key& static_key, certificate& proof,
        const act2& in) NOEXCEPT;

    /// Seal a frame (heading then payload) into out, where
    /// out = sealed_heading_size + sealed_size(frame size - heading_size).
    /// False if the send nonce is exhausted.
    bool encrypt(const std::span<uint8_t>& out, const span& frame) NOEXCEPT;

    /// Open a sealed heading (in = sealed_heading_size) into plain.
    /// False if the tag does not authenticate or nonce is exhausted.
    bool decrypt_heading(const std::span<uint8_t>& plain,
        const span& in) NOEXCEPT;

    /// Open a sealed payload (in = sealed_size(plain size)) into plain.
    /// False if a tag does not authenticate or nonce is exhausted.
    bool decrypt_payload(const std::span<uint8_t>& plain,
        const span& in) NOEXCEPT;

private:
    // ChaCha20-Poly1305 with a counter nonce, the last value reserved.
    class state
    {
    public:
        state(const system::hash_digest& secret) NOEXCEPT;
        bool encrypt(const std::span<uint8_t>& out, const span& aad,
            const span& plain) NOEXCEPT;
        bool decrypt(const std::span<uint8_t>& plain, const span& aad,
            const span& in) NOEXCEPT;

    private:
        system::chacha20_poly1305 aead_;
        uint64_t nonce_{};
    };

    void mix_hash(const span& data) NOEXCEPT;
    void mix_key(const system::hash_digest& material) NOEXCEPT;
    bool encrypt_and_hash(const std::span<uint8_t>& out,
        const span& plain) NOEXCEPT;
    bool decrypt_and_hash(const std::span<uint8_t>& plain,
        const span& in) NOEXCEPT;
    void split(bool initiator) NOEXCEPT;

    // Keys.
    system::ec_secret secret_;
    key public_;
    system::ec_secret static_secret_{};
    key static_{};
    certificate proof_{};

    // Symmetric state.
    system::hash_digest chaining_;
    system::hash_digest hash_;
    std::optional<state> handshake_{};

    // Transport.
    std::optional<state> send_{};
    std::optional<state> receive_{};
};

} // namespace noise
} // namespace network
} // namespace libbitcoin

#endif
