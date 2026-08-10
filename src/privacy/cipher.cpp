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
#include <bitcoin/network/privacy/cipher.hpp>

#include <random>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

using namespace system;

BC_PUSH_WARNING(NO_ARRAY_INDEXING)
BC_PUSH_WARNING(NO_DYNAMIC_ARRAY_INDEXING)

// bip324 key derivation salt prefix.
constexpr char salt_label[] = "bitcoin_v2_shared_secret";

// This is to be used only for ephemeral network session keys.
// Gather 256 bits of operating system entropy, conditioned by sha256.
static hash_digest entropy() NOEXCEPT
{
    using word = std::random_device::result_type;
    constexpr auto words = hash_size / sizeof(word);

    std::random_device device{};
    data_array<words * sizeof(word)> seed{};
    auto it = seed.begin();

    for (size_t count{}; count < words; ++count)
    {
        const auto value = to_little_endian(device());
        it = std::copy(value.begin(), value.end(), it);
    }

    return sha256_hash(seed);
}

cipher::cipher() NOEXCEPT
  : secret_{}, key_{}
{
    // A secret outside the group order is astronomically improbable.
    do
    {
        secret_ = entropy();
    }
    while (!ellswift::create(key_, secret_, entropy()));
}

cipher::cipher(const ec_secret& secret, const key& public_key) NOEXCEPT
  : secret_(secret), key_(public_key)
{
}

const cipher::key& cipher::public_key() const NOEXCEPT
{
    return key_;
}

bool cipher::initialize(const key& theirs, uint32_t identifier,
    bool initiating) NOEXCEPT
{
    // bip324
    // The initiating party is party a in the ecdh tagged hash.
    hash_digest ecdh{};
    const auto& key_a = initiating ? key_ : theirs;
    const auto& key_b = initiating ? theirs : key_;
    if (!ellswift::exchange(ecdh, secret_, key_a, key_b, !initiating))
        return false;

    // bip324
    // salt = "bitcoin_v2_shared_secret" + network magic bytes.
    const auto salt = build_chunk(
    {
        to_chunk(salt_label),
        to_little_endian(identifier)
    });

    // bip324
    // HKDF-SHA256 extraction, with per-key expansion by label.
    const auto prk = hkdf<sha256>::extract(ecdh, salt);
    ecdh = {};

    chacha20::secret okm{};
    hkdf<sha256>::expand(okm, prk, to_chunk("initiator_L"));
    (initiating ? send_length_ : receive_length_).emplace(okm, rekey_interval);
    hkdf<sha256>::expand(okm, prk, to_chunk("initiator_P"));
    (initiating ? send_packet_ : receive_packet_).emplace(okm, rekey_interval);
    hkdf<sha256>::expand(okm, prk, to_chunk("responder_L"));
    (initiating ? receive_length_ : send_length_).emplace(okm, rekey_interval);
    hkdf<sha256>::expand(okm, prk, to_chunk("responder_P"));
    (initiating ? receive_packet_ : send_packet_).emplace(okm, rekey_interval);

    // bip324
    // The initiator terminator is the first half of the expansion.
    hkdf<sha256>::expand(okm, prk, to_chunk("garbage_terminators"));
    auto& initiator = initiating ? send_terminator_ : receive_terminator_;
    auto& responder = initiating ? receive_terminator_ : send_terminator_;
    initiator = slice<zero, terminator_size>(okm);
    responder = slice<terminator_size, chacha20::secret_size>(okm);

    hkdf<sha256>::expand(session_id_, prk, to_chunk("session_id"));
    okm = {};

    // The secret is no longer required (or usable).
    secret_ = {};
    return true;
}

const cipher::terminator& cipher::send_terminator() const NOEXCEPT
{
    return send_terminator_;
}

const cipher::terminator& cipher::receive_terminator() const NOEXCEPT
{
    return receive_terminator_;
}

const hash_digest& cipher::session_id() const NOEXCEPT
{
    return session_id_;
}

void cipher::encrypt(std::span<const uint8_t> contents,
    std::span<const uint8_t> aad, bool ignore, std::span<uint8_t> out) NOEXCEPT
{
    BC_ASSERT(out.size() == contents.size() + expansion);
    BC_ASSERT(contents.size() <= maximum_content);

    // bip324
    // The three byte little-endian contents length is independently encrypted.
    const data_array<length_size> length
    {
        narrow_cast<uint8_t>(bit_and<size_t>(contents.size(), 0xff)),
        narrow_cast<uint8_t>(bit_and<size_t>(shift_right(contents.size(), 8u), 0xff)),
        narrow_cast<uint8_t>(bit_and<size_t>(shift_right(contents.size(), 16u), 0xff))
    };

    send_length_->crypt(length, out.first(length_size));

    // bip324
    // The header byte (ignore bit) is prepended to contents for encryption.
    const data_array<header_size> header{ ignore ? ignore_bit : 0x00_u8 };
    data_chunk plain{};
    plain.reserve(header_size + contents.size());
    plain.insert(plain.end(), header.begin(), header.end());
    plain.insert(plain.end(), contents.begin(), contents.end());
    send_packet_->encrypt(plain, aad, out.subspan(length_size));
}

size_t cipher::decrypt_length(std::span<const uint8_t> in) NOEXCEPT
{
    BC_ASSERT(in.size() == length_size);

    data_array<length_size> length{};
    receive_length_->crypt(in, length);

    return
        (static_cast<size_t>(length[0])) |
        (static_cast<size_t>(length[1]) << 8u) |
        (static_cast<size_t>(length[2]) << 16u);
}

bool cipher::decrypt(std::span<uint8_t> contents, std::span<const uint8_t> aad,
    bool& ignore, std::span<const uint8_t> in) NOEXCEPT
{
    BC_ASSERT(in.size() == contents.size() + header_size + tag_size);

    data_chunk plain(header_size + contents.size());
    if (!receive_packet_->decrypt(plain, aad, in))
        return false;

    ignore = to_bool(bit_and(plain.front(), ignore_bit));
    std::copy(std::next(plain.begin()), plain.end(), contents.begin());
    return true;
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace privacy
} // namespace network
} // namespace libbitcoin
