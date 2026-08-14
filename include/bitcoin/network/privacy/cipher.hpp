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
#ifndef LIBBITCOIN_NETWORK_PRIVACY_CIPHER_HPP
#define LIBBITCOIN_NETWORK_PRIVACY_CIPHER_HPP

#include <optional>
#include <span>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

/// The bip324 (v2) transport session cipher.
/// This is to be used only for ephemeral network session keys.
/// Construction generates an ephemeral keypair. Once the peer key is known,
/// initialize() derives the directional packet ciphers, garbage terminators
/// and session identifier. Not thread safe.
class BCT_API cipher final
{
public:
    DELETE_COPY_MOVE(cipher);

    /// bip324 constants.
    static constexpr size_t key_size = system::ec_ellswift_size;
    static constexpr size_t terminator_size = 16;
    static constexpr size_t length_size = 3;
    static constexpr size_t header_size = 1;
    static constexpr size_t tag_size = system::chacha20_poly1305::expansion;
    static constexpr size_t maximum_garbage = 4095;
    static constexpr size_t maximum_content = 0x00ffffff;
    static constexpr uint32_t rekey_interval = 224;
    static constexpr uint8_t ignore_bit = 0x80;

    /// Ciphertext expansion (encrypted length, header and tag).
    static constexpr size_t expansion = length_size + header_size + tag_size;

    typedef system::data_array<key_size> key;
    typedef system::data_array<terminator_size> terminator;

    /// Generate an ephemeral keypair for the session.
    cipher() NOEXCEPT;

    /// Construct from a given keypair (deterministic, for test vectors).
    cipher(const system::ec_secret& secret, const key& public_key) NOEXCEPT;

    /// Own public key (ElligatorSwift encoded), sent in the clear.
    const key& public_key() const NOEXCEPT;

    /// Derive session state from the peer public key (false on ecdh failure).
    /// Set initiating if this side opened the connection.
    bool initialize(const key& theirs, uint32_t identifier,
        bool initiating) NOEXCEPT;

    /// Session state (valid after successful initialize).
    const terminator& send_terminator() const NOEXCEPT;
    const terminator& receive_terminator() const NOEXCEPT;
    const system::hash_digest& session_id() const NOEXCEPT;

    /// Encrypt contents into out (out = contents size + expansion).
    void encrypt(std::span<const uint8_t> contents,
        std::span<const uint8_t> aad, bool ignore,
        std::span<uint8_t> out) NOEXCEPT;

    /// Decrypt an encrypted packet length (advances the length cipher).
    size_t decrypt_length(std::span<const uint8_t> in) NOEXCEPT;

    /// Decrypt packet remainder into contents, setting the decoy indicator.
    /// in = header + contents + tag, contents = in size - header - tag.
    /// False if the tag does not authenticate.
    bool decrypt(std::span<uint8_t> contents, std::span<const uint8_t> aad,
        bool& ignore, std::span<const uint8_t> in) NOEXCEPT;

private:
    system::ec_secret secret_;
    key key_;
    terminator send_terminator_{};
    terminator receive_terminator_{};
    system::hash_digest session_id_{};
    std::optional<system::fschacha20> send_length_{};
    std::optional<system::fschacha20> receive_length_{};
    std::optional<system::fschacha20_poly1305> send_packet_{};
    std::optional<system::fschacha20_poly1305> receive_packet_{};
};

} // namespace privacy
} // namespace network
} // namespace libbitcoin

#endif
