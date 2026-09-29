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

#include <string_view>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/codec.hpp>
#include <bitcoin/network/tls/constants.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

using namespace system;

constexpr std::string_view label_prefix{ "tls13 " };
constexpr size_t aes_key_size = 16;
constexpr size_t chacha_key_size = 32;

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

template <size_t Size>
static data_array<Size> expand(const schedule::secret& key,
    std::string_view label, const const_byte_span& context) NOEXCEPT
{
    data_array<Size> out{};
    hkdf<sha256>::expand(out, key, make_label(Size, label, context));
    return out;
}

schedule::secret schedule::extract(const secret& salt,
    const const_byte_span& material) NOEXCEPT
{
    const data_slice input(material.begin(), material.end());
    return hkdf<sha256>::extract(input, salt);
}

schedule::secret schedule::expand_label(const secret& key,
    std::string_view label, const const_byte_span& context) NOEXCEPT
{
    return expand<sizeof(secret)>(key, label, context);
}

schedule::secret schedule::derive_secret(const secret& key,
    std::string_view label, const secret& transcript) NOEXCEPT
{
    return expand_label(key, label, transcript);
}

schedule::secret schedule::early_secret() NOEXCEPT
{
    constexpr secret zeros{};
    return extract(zeros, zeros);
}

schedule::secret schedule::handshake_secret(const secret& early,
    const const_byte_span& shared) NOEXCEPT
{
    const auto empty = sha256_hash(data_chunk{});
    return extract(derive_secret(early, "derived", empty), shared);
}

schedule::secret schedule::master_secret(const secret& handshake) NOEXCEPT
{
    constexpr secret zeros{};
    const auto empty = sha256_hash(data_chunk{});
    return extract(derive_secret(handshake, "derived", empty), zeros);
}

data_chunk schedule::traffic_key(const secret& traffic,
    uint16_t suite) NOEXCEPT
{
    if (suite == aes_128_gcm_sha256)
        return to_chunk(expand<aes_key_size>(traffic, "key", {}));

    BC_ASSERT(suite == chacha20_poly1305_sha256);
    return to_chunk(expand<chacha_key_size>(traffic, "key", {}));
}

schedule::iv schedule::traffic_iv(const secret& traffic) NOEXCEPT
{
    return expand<sizeof(iv)>(traffic, "iv", {});
}

schedule::secret schedule::finished(const secret& traffic,
    const secret& transcript) NOEXCEPT
{
    const auto key = expand_label(traffic, "finished", {});
    return hmac<sha256>::code(transcript, key);
}

schedule::secret schedule::update(const secret& traffic) NOEXCEPT
{
    return expand_label(traffic, "traffic upd", {});
}

} // namespace tls
} // namespace network
} // namespace libbitcoin
