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

// key derivation salt prefix.
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
  : key_{}, secret_{}
{
    // A secret outside the group order is astronomically improbable.
    do
    {
        secret_ = entropy();
    }
    while (!ellswift::create(key_, secret_, entropy()));
}

cipher::cipher(const ec_secret& secret, const key& public_key) NOEXCEPT
  : key_(public_key), secret_(secret)
{
}

const cipher::key& cipher::public_key() const NOEXCEPT
{
    return key_;
}

bool cipher::initialize(const key& theirs, uint32_t identifier,
    bool initiating) NOEXCEPT
{
    // The initiating party is party a in the ecdh tagged hash.
    hash_digest ecdh{};
    const auto& key_a = initiating ? key_ : theirs;
    const auto& key_b = initiating ? theirs : key_;
    if (!ellswift::exchange(ecdh, secret_, key_a, key_b, !initiating))
        return false;

    // salt = "bitcoin_v2_shared_secret" + network magic bytes.
    const auto salt = build_chunk(
    {
        to_chunk(salt_label),
        to_little_endian(identifier)
    });

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

    // The three byte little-endian content length is independently encrypted.
    const auto length = to_little_endian_size<length_size>(contents.size());
    send_length_->crypt(length, out.first(length_size));

    // The header byte (ignore bit) is prepended to contents for encryption.
    const data_array<header_size> header{ ignore ? ignore_bit : 0x00_u8 };
    send_packet_->encrypt(header, contents, aad, out.subspan(length_size));
}

size_t cipher::decrypt_length(std::span<const uint8_t> in) NOEXCEPT
{
    BC_ASSERT(in.size() == length_size);

    data_array<length_size> length{};
    receive_length_->crypt(in, length);
    return from_little_array<size_t>(length);
}

bool cipher::decrypt(const std::span<uint8_t>& plain,
    const std::span<const uint8_t>& aad, bool& ignore,
    const std::span<const uint8_t>& in) NOEXCEPT
{
    BC_ASSERT(in.size() == plain.size() + tag_size);

    if (!receive_packet_->decrypt(plain, aad, in))
        return false;

    ignore = to_bool(bit_and(plain.front(), ignore_bit));
    return true;
}

BC_POP_WARNING()
BC_POP_WARNING()

} // namespace privacy
} // namespace network
} // namespace libbitcoin
