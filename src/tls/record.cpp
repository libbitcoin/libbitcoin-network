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

#include <algorithm>
#include <iterator>
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

void record::set_secret(uint16_t suite,
    const schedule::secret& traffic) NOEXCEPT
{
    suite_ = suite;
    sequence_ = zero;
    iv_ = schedule::traffic_iv(traffic);
    const auto bytes = schedule::traffic_key(traffic, suite);

    if (suite == aes_128_gcm_sha256)
    {
        aes128_gcm::secret key{};
        std::copy(bytes.begin(), bytes.end(), key.begin());
        aes_.emplace(key);
        chacha_.reset();
        return;
    }

    BC_ASSERT(suite == chacha20_poly1305_sha256);
    chacha20::secret key{};
    std::copy(bytes.begin(), bytes.end(), key.begin());
    chacha_.emplace(key);
    aes_.reset();
}

bool record::is_protected() const NOEXCEPT
{
    return aes_.has_value() || chacha_.has_value();
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
        out.insert(out.end(), header.data().begin(), header.data().end());
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
    out.insert(out.end(), header.data().begin(), header.data().end());
    out.resize(out.size() + size);

    const auto aad = header.data();
    const auto cipher = byte_span{ out }.subspan(start + record_header_size);
    const auto value = next_nonce();

    if (aes_)
    {
        aes_->encrypt(content, inner_type, aad, value, cipher);
        return;
    }

    const auto nonce32 = from_little<uint32_t, zero>(value);
    const auto nonce64 = from_little<uint64_t, sizeof(uint32_t)>(value);
    chacha_->encrypt(content, inner_type, aad, nonce32, nonce64, cipher);
}

bool record::open(uint8_t& type, data_chunk& content,
    const const_byte_span& header, const const_byte_span& fragment) NOEXCEPT
{
    BC_ASSERT(is_protected());
    if (fragment.size() <= tag_size)
        return false;

    data_chunk plain(fragment.size() - tag_size);
    const auto value = next_nonce();

    auto valid = false;
    if (aes_)
    {
        valid = aes_->decrypt(plain, header, value, fragment);
    }
    else
    {
        const auto nonce32 = from_little<uint32_t, zero>(value);
        const auto nonce64 = from_little<uint64_t, sizeof(uint32_t)>(value);
        valid = chacha_->decrypt(plain, header, nonce32, nonce64, fragment);
    }

    if (!valid)
        return false;

    // The inner type is the last nonzero byte, followed by zero padding.
    const auto last = std::find_if(plain.rbegin(), plain.rend(),
        [](uint8_t byte) NOEXCEPT { return !is_zero(byte); });
    if (last == plain.rend())
        return false;

    type = *last;
    plain.resize(std::distance(last, plain.rend()) - one);
    content = std::move(plain);
    return true;
}

BC_POP_WARNING()

} // namespace tls
} // namespace network
} // namespace libbitcoin
