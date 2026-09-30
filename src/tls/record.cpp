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
#include <bitcoin/network/tls/record.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/codec.hpp>
#include <bitcoin/network/tls/constants.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

using namespace system;

BC_PUSH_WARNING(NO_ARRAY_INDEXING)

record::record() NOEXCEPT
{
}

record::~record() NOEXCEPT
{
    wipe(iv_);
}

template <typename Cipher, typename Secret>
static void emplace(auto& cipher, const data_chunk& bytes) NOEXCEPT
{
    Secret key{};
    std::copy(bytes.cbegin(), bytes.cend(), key.begin());
    cipher.template emplace<Cipher>(key);
    wipe(key);
}

void record::set_secret(uint16_t suite,
    const schedule::secret& traffic) NOEXCEPT
{
    const schedule keys{ suite };
    sequence_ = zero;
    iv_ = keys.traffic_iv(traffic);
    auto bytes = keys.traffic_key(traffic);

    if (suite == aes_128_gcm_sha256)
        emplace<aes128_gcm, aes128_gcm::secret>(cipher_, bytes);
    else if (suite == aes_256_gcm_sha384)
        emplace<aes256_gcm, aes256_gcm::secret>(cipher_, bytes);
    else
        emplace<chacha20_poly1305, chacha20::secret>(cipher_, bytes);

    wipe(bytes.data(), bytes.size());
}

bool record::is_protected() const NOEXCEPT
{
    return !std::holds_alternative<std::monostate>(cipher_);
}

uint64_t record::sequence() const NOEXCEPT
{
    return sequence_;
}

// The per-record nonce is the iv xored with the padded sequence (5.3).
record::nonce record::next_nonce() NOEXCEPT
{
    auto value = iv_;
    const auto sequence = to_big_endian(sequence_++);
    const auto offset = value.size() - sequence.size();
    for (size_t byte{}; byte < sequence.size(); ++byte)
        value[offset + byte] = bit_xor(value[offset + byte], sequence[byte]);

    return value;
}

void record::seal(data_chunk& out, uint8_t type,
    const const_byte_span& content) NOEXCEPT
{
    BC_ASSERT(content.size() <= maximum_plaintext);

    if (!is_protected())
    {
        writer header{};
        header.write_8(type);
        header.write_16(legacy_version);
        header.write_vector_16(content);
        out.insert(out.cend(), header.data().cbegin(), header.data().cend());
        return;
    }

    // TLSInnerPlaintext is the content and its type (without padding).
    const data_array<one> inner_type{ type };
    const auto size = content.size() + inner_type.size() + tag_size;

    writer header{};
    header.write_8(content::application_data);
    header.write_16(legacy_version);
    header.write_16(possible_narrow_cast<uint16_t>(size));

    const auto start = out.size();
    out.insert(out.cend(), header.data().cbegin(), header.data().cend());
    out.resize(out.size() + size);

    const auto aad = header.data();
    const auto text = byte_span{ out }.subspan(start + record_header_size);
    const auto value = next_nonce();

    std::visit([&](auto& cipher) NOEXCEPT
    {
        using type = std::decay_t<decltype(cipher)>;
        if constexpr (is_same_type<type, chacha20_poly1305>)
        {
            const auto nonce32 = from_little<uint32_t, zero>(value);
            const auto nonce64 = from_little<uint64_t, sizeof(uint32_t)>(value);
            cipher.encrypt(content, inner_type, aad, nonce32, nonce64, text);
        }
        else if constexpr (!is_same_type<type, std::monostate>)
        {
            cipher.encrypt(content, inner_type, aad, value, text);
        }
    }, cipher_);
}

bool record::open(uint8_t& type, data_chunk& content,
    const const_byte_span& header, const const_byte_span& fragment) NOEXCEPT
{
    BC_ASSERT(is_protected());
    if (fragment.size() <= tag_size)
        return false;

    data_chunk plain(fragment.size() - tag_size);
    const auto value = next_nonce();

    const auto valid = std::visit([&](auto& cipher) NOEXCEPT
    {
        using type = std::decay_t<decltype(cipher)>;
        if constexpr (is_same_type<type, chacha20_poly1305>)
        {
            const auto nonce32 = from_little<uint32_t, zero>(value);
            const auto nonce64 = from_little<uint64_t, sizeof(uint32_t)>(value);
            return cipher.decrypt(plain, header, nonce32, nonce64, fragment);
        }
        else if constexpr (is_same_type<type, std::monostate>)
        {
            return false;
        }
        else
        {
            return cipher.decrypt(plain, header, value, fragment);
        }
    }, cipher_);

    if (!valid)
        return false;

    // The inner type is the last nonzero byte, followed by zero padding.
    const auto last = std::find_if(plain.crbegin(), plain.crend(),
        [](uint8_t byte) NOEXCEPT
        {
            return is_nonzero(byte);
        });

    if (last == plain.crend())
    {
        type = 0;
        content.clear();
        return true;
    }

    type = *last;
    plain.resize(sub1(std::distance(last, plain.crend())));
    content = std::move(plain);
    return true;
}

BC_POP_WARNING()

} // namespace tls
} // namespace network
} // namespace libbitcoin
