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
#include <bitcoin/network/noise/cipher.hpp>

#include <algorithm>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace noise {

using namespace system;

constexpr char protocol_name[] = "Noise_NX_Secp256k1+EllSwift_ChaChaPoly_SHA256";

static void generate(ec_secret& secret, cipher::key& public_key) NOEXCEPT
{
    // A secret outside the group order is astronomically improbable.
    hash_digest tweak{};
    do
    {
        maybe_random::fill(secret);
        maybe_random::fill(tweak);
    }
    while (!ellswift::create(public_key, secret, tweak));
}

// Construction.
// ----------------------------------------------------------------------------

cipher::cipher() NOEXCEPT
  : cipher(ec_secret{}, key{})
{
    generate(secret_, public_);
}

cipher::cipher(const ec_secret& secret, const key& public_key) NOEXCEPT
  : secret_(secret),
    public_(public_key),
    chaining_(sha256_hash(to_chunk(protocol_name))),
    hash_(sha256_hash(chaining_))
{
}

cipher::cipher(const ec_secret& static_secret, const key& static_key,
    const certificate& proof) NOEXCEPT
  : cipher(ec_secret{}, key{}, static_secret, static_key, proof)
{
    generate(secret_, public_);
}

cipher::cipher(const ec_secret& secret, const key& public_key,
    const ec_secret& static_secret, const key& static_key,
    const certificate& proof) NOEXCEPT
  : secret_(secret),
    public_(public_key),
    static_secret_(static_secret),
    static_(static_key),
    proof_(proof),
    chaining_(sha256_hash(to_chunk(protocol_name))),
    hash_(sha256_hash(chaining_))
{
}

const cipher::key& cipher::public_key() const NOEXCEPT
{
    return public_;
}

const hash_digest& cipher::handshake_hash() const NOEXCEPT
{
    return hash_;
}

// Handshake.
// ----------------------------------------------------------------------------

// -> e
void cipher::initiate(act1& out) NOEXCEPT
{
    out = public_;
    mix_hash(public_);
    mix_hash({});
}

// <- e, ee, s, es, SIGNATURE_NOISE_MESSAGE
bool cipher::respond(act2& out, const act1& in) NOEXCEPT
{
    mix_hash(in);
    mix_hash({});

    const std::span<uint8_t> message{ out };
    std::copy(public_.begin(), public_.end(), out.begin());
    mix_hash(public_);

    hash_digest ecdh{};
    if (!ellswift::exchange(ecdh, secret_, in, public_, true))
        return false;

    mix_key(ecdh);
    if (!encrypt_and_hash(message.subspan(key_size, key_size + tag_size),
        static_))
        return false;

    if (!ellswift::exchange(ecdh, static_secret_, in, static_, true))
        return false;

    mix_key(ecdh);
    ecdh = {};
    if (!encrypt_and_hash(message.subspan(key_size + key_size + tag_size),
        proof_))
        return false;

    split(false);
    return true;
}

bool cipher::complete(key& static_key, certificate& proof,
    const act2& in) NOEXCEPT
{
    const span message{ in };
    const auto theirs = slice<zero, key_size>(in);
    mix_hash(theirs);

    hash_digest ecdh{};
    if (!ellswift::exchange(ecdh, secret_, public_, theirs, false))
        return false;

    mix_key(ecdh);
    if (!decrypt_and_hash(static_key, message.subspan(key_size,
        key_size + tag_size)))
        return false;

    if (!ellswift::exchange(ecdh, secret_, public_, static_key, false))
        return false;

    mix_key(ecdh);
    ecdh = {};
    if (!decrypt_and_hash(proof, message.subspan(key_size + key_size +
        tag_size)))
        return false;

    split(true);
    return true;
}

// Transport.
// ----------------------------------------------------------------------------

bool cipher::encrypt(const std::span<uint8_t>& out, const span& frame) NOEXCEPT
{
    BC_ASSERT(frame.size() >= heading_size);
    const auto payload = frame.subspan(heading_size);
    BC_ASSERT(out.size() == sealed_heading_size + sealed_size(payload.size()));

    if (!send_->encrypt(out.first(sealed_heading_size), {},
        frame.first(heading_size)))
        return false;

    auto sealed = out.subspan(sealed_heading_size);
    for (size_t start{}; start < payload.size(); start += chunk_size)
    {
        const auto size = std::min(chunk_size, payload.size() - start);
        if (!send_->encrypt(sealed.first(size + tag_size), {},
            payload.subspan(start, size)))
            return false;

        sealed = sealed.subspan(size + tag_size);
    }

    return true;
}

bool cipher::decrypt_heading(const std::span<uint8_t>& plain,
    const span& in) NOEXCEPT
{
    BC_ASSERT(plain.size() == heading_size);
    BC_ASSERT(in.size() == sealed_heading_size);
    return receive_->decrypt(plain, {}, in);
}

bool cipher::decrypt_payload(const std::span<uint8_t>& plain,
    const span& in) NOEXCEPT
{
    BC_ASSERT(in.size() == sealed_size(plain.size()));

    auto sealed = in;
    for (size_t start{}; start < plain.size(); start += chunk_size)
    {
        const auto size = std::min(chunk_size, plain.size() - start);
        if (!receive_->decrypt(plain.subspan(start, size), {},
            sealed.first(size + tag_size)))
            return false;

        sealed = sealed.subspan(size + tag_size);
    }

    return true;
}

// Symmetric state (private).
// ----------------------------------------------------------------------------

void cipher::mix_hash(const span& data) NOEXCEPT
{
    hash_ = sha256_hash2(hash_, { data.begin(), data.end() });
}

void cipher::mix_key(const hash_digest& material) NOEXCEPT
{
    const auto keys = hkdf<sha256>::key<two * hash_size>(material, chaining_,
        {});
    chaining_ = slice<zero, hash_size>(keys);
    handshake_.emplace(slice<hash_size, two * hash_size>(keys));
}

bool cipher::encrypt_and_hash(const std::span<uint8_t>& out,
    const span& plain) NOEXCEPT
{
    if (!handshake_->encrypt(out, hash_, plain))
        return false;

    mix_hash(out);
    return true;
}

bool cipher::decrypt_and_hash(const std::span<uint8_t>& plain,
    const span& in) NOEXCEPT
{
    if (!handshake_->decrypt(plain, hash_, in))
        return false;

    mix_hash(in);
    return true;
}

// The first key is the initiator's sending key.
void cipher::split(bool initiator) NOEXCEPT
{
    const auto keys = hkdf<sha256>::key<two * hash_size>({}, chaining_, {});
    const auto first = slice<zero, hash_size>(keys);
    const auto second = slice<hash_size, two * hash_size>(keys);
    send_.emplace(initiator ? first : second);
    receive_.emplace(initiator ? second : first);

    // The secrets are no longer required.
    secret_ = {};
    static_secret_ = {};
    handshake_.reset();
}

// Cipher state (private).
// ----------------------------------------------------------------------------

cipher::state::state(const hash_digest& secret) NOEXCEPT
  : aead_(secret)
{
}

bool cipher::state::encrypt(const std::span<uint8_t>& out, const span& aad,
    const span& plain) NOEXCEPT
{
    if (nonce_ == max_uint64)
        return false;

    aead_.encrypt(plain, aad, 0, nonce_++, out);
    return true;
}

bool cipher::state::decrypt(const std::span<uint8_t>& plain, const span& aad,
    const span& in) NOEXCEPT
{
    if (nonce_ == max_uint64 || !aead_.decrypt(plain, aad, 0, nonce_, in))
        return false;

    ++nonce_;
    return true;
}

} // namespace noise
} // namespace network
} // namespace libbitcoin
