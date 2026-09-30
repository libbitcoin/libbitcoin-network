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
#include <bitcoin/network/tls/schedule.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/codec.hpp>
#include <bitcoin/network/tls/constants.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

using namespace system;

constexpr size_t aes128_key_size = 16;
constexpr size_t aes256_key_size = 32;
constexpr size_t chacha_key_size = 32;
constexpr std::string_view label_prefix{ "tls13 " };

// HkdfLabel (7.1).
static data_chunk make_label(size_t size, std::string_view label,
    const const_byte_span& context) NOEXCEPT
{
    std::string full{ label_prefix };
    full.append(label);

    writer out{};
    out.write_16(possible_narrow_cast<uint16_t>(size));
    out.write_vector_8(to_chunk(full));
    out.write_vector_8(context);
    return out.data();
}

template <typename Algorithm, size_t Size>
static data_chunk expand(const schedule::secret& key, std::string_view label,
    const const_byte_span& context) NOEXCEPT
{
    typename Algorithm::digest_t prk{};
    BC_ASSERT(key.size() == prk.size());
    std::copy(key.cbegin(), key.cend(), prk.begin());

    data_array<Size> out{};
    hkdf<Algorithm>::expand(out, prk, make_label(Size, label, context));
    const auto value = to_chunk(out);
    wipe(prk);
    wipe(out);
    return value;
}

template <typename Algorithm>
static data_chunk code(const schedule::secret& key,
    const schedule::secret& data) NOEXCEPT
{
    return to_chunk(hmac<Algorithm>::code(data, key));
}

// schedule
// ----------------------------------------------------------------------------

schedule::schedule(uint16_t suite) NOEXCEPT
  : suite_(suite)
{
}

bool schedule::is_sha384() const NOEXCEPT
{
    return suite_ == aes_256_gcm_sha384;
}

size_t schedule::size() const NOEXCEPT
{
    return is_sha384() ? array_count<sha512_384::digest_t> :
        array_count<sha256::digest_t>;
}

schedule::secret schedule::hash(const const_byte_span& data) const NOEXCEPT
{
    if (is_sha384())
        return to_chunk(accumulator<sha512_384>::hash(data.size(),
            data.data()));

    return to_chunk(accumulator<sha256>::hash(data.size(), data.data()));
}

schedule::secret schedule::extract(const secret& salt,
    const const_byte_span& material) const NOEXCEPT
{
    const data_slice input(material.begin(), material.end());
    if (is_sha384())
        return to_chunk(hkdf<sha512_384>::extract(input, salt));

    return to_chunk(hkdf<sha256>::extract(input, salt));
}

schedule::secret schedule::expand_label(const secret& key,
    std::string_view label, const const_byte_span& context) const NOEXCEPT
{
    if (is_sha384())
        return expand<sha512_384, 48>(key, label, context);

    return expand<sha256, 32>(key, label, context);
}

schedule::secret schedule::derive_secret(const secret& key,
    std::string_view label, const secret& transcript) const NOEXCEPT
{
    return expand_label(key, label, transcript);
}

schedule::secret schedule::early_secret() const NOEXCEPT
{
    const secret zeros(size());
    return extract(zeros, zeros);
}

schedule::secret schedule::handshake_secret(const secret& early,
    const const_byte_span& shared) const NOEXCEPT
{
    auto derived = derive_secret(early, "derived", hash({}));
    const auto out = extract(derived, shared);
    wipe(derived.data(), derived.size());
    return out;
}

schedule::secret schedule::master_secret(
    const secret& handshake) const NOEXCEPT
{
    const secret zeros(size());
    auto derived = derive_secret(handshake, "derived", hash({}));
    const auto out = extract(derived, zeros);
    wipe(derived.data(), derived.size());
    return out;
}

data_chunk schedule::traffic_key(const secret& traffic) const NOEXCEPT
{
    if (is_sha384())
        return expand<sha512_384, aes256_key_size>(traffic, "key", {});

    if (suite_ == aes_128_gcm_sha256)
        return expand<sha256, aes128_key_size>(traffic, "key", {});

    BC_ASSERT(suite_ == chacha20_poly1305_sha256);
    return expand<sha256, chacha_key_size>(traffic, "key", {});
}

schedule::iv schedule::traffic_iv(const secret& traffic) const NOEXCEPT
{
    iv out{};
    const auto bytes = is_sha384() ?
        expand<sha512_384, sizeof(iv)>(traffic, "iv", {}) :
        expand<sha256, sizeof(iv)>(traffic, "iv", {});

    std::copy(bytes.cbegin(), bytes.cend(), out.begin());
    return out;
}

schedule::secret schedule::finished(const secret& traffic,
    const secret& transcript) const NOEXCEPT
{
    auto key = expand_label(traffic, "finished", {});
    const auto out = is_sha384() ? code<sha512_384>(key, transcript) :
        code<sha256>(key, transcript);

    wipe(key.data(), key.size());
    return out;
}

schedule::secret schedule::update(const secret& traffic) const NOEXCEPT
{
    return expand_label(traffic, "traffic upd", {});
}

// transcript
// ----------------------------------------------------------------------------

transcript::transcript() NOEXCEPT
{
}

void transcript::reset(uint16_t suite) NOEXCEPT
{
    if (suite == aes_256_gcm_sha384)
        state_.emplace<accumulator<sha512_384>>();
    else
        state_.emplace<accumulator<sha256>>();
}

void transcript::write(const const_byte_span& message) NOEXCEPT
{
    std::visit([&](auto& state) NOEXCEPT
    {
        state.write(message.size(), message.data());
    }, state_);
}

schedule::secret transcript::hash() const NOEXCEPT
{
    return std::visit([](auto state) NOEXCEPT
    {
        return to_chunk(state.flush());
    }, state_);
}

} // namespace tls
} // namespace network
} // namespace libbitcoin
