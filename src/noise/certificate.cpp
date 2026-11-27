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
#include <bitcoin/network/noise/certificate.hpp>

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace noise {

using namespace system;

constexpr data_array<sizeof(uint16_t)> authority_prefix{ 0x01, 0x00 };
constexpr size_t checked_size = certificate::authority_size + checksum_default_size;

static data_array<certificate::header_size> header(uint32_t valid_from,
    uint32_t not_valid_after) NOEXCEPT
{
    return splice(to_little_endian(certificate::version),
        to_little_endian(valid_from), to_little_endian(not_valid_after));
}

static hash_digest signature_hash(const ec_xonly& server, uint32_t valid_from,
    uint32_t not_valid_after) NOEXCEPT
{
    return sha256_hash2(header(valid_from, not_valid_after), server);
}

bool certificate::to_xonly(ec_xonly& out, const ec_secret& secret) NOEXCEPT
{
    ec_compressed point{};
    if (!secret_to_public(point, secret))
        return false;

    out = to_xonly(point);
    return true;
}

bool certificate::to_xonly(ec_xonly& out, const cipher::key& key) NOEXCEPT
{
    ec_compressed point{};
    if (!ellswift::decode(point, key))
        return false;

    out = to_xonly(point);
    return true;
}

ec_xonly certificate::to_xonly(const ec_compressed& key) NOEXCEPT
{
    return slice<one, ec_compressed_size>(key);
}

bool certificate::sign(data& out, const ec_secret& authority,
    const ec_xonly& server, uint32_t valid_from,
    uint32_t not_valid_after) NOEXCEPT
{
    hash_digest auxiliary{};
    maybe_random::fill(auxiliary);
    return sign(out, authority, server, valid_from, not_valid_after,
        auxiliary);
}

bool certificate::sign(data& out, const ec_secret& authority,
    const ec_xonly& server, uint32_t valid_from, uint32_t not_valid_after,
    const hash_digest& auxiliary) NOEXCEPT
{
    if (not_valid_after < valid_from)
        return false;

    ec_signature signature{};
    const auto hash = signature_hash(server, valid_from, not_valid_after);
    if (!schnorr::sign(signature, authority, hash, auxiliary))
        return false;

    out = splice(header(valid_from, not_valid_after), signature);
    return true;
}

bool certificate::verify(const data& in, const ec_xonly& server,
    const ec_xonly& authority, uint32_t now) NOEXCEPT
{
    const auto from = valid_from(in);
    const auto after = not_valid_after(in);
    if (from_little_array<uint16_t>(slice<zero, sizeof(uint16_t)>(in)) !=
        version || now < from || now > after)
        return false;

    const auto hash = signature_hash(server, from, after);
    const auto signature = slice<header_size, cipher::certificate_size>(in);
    return schnorr::verify_signature(authority, hash, signature);
}

uint32_t certificate::valid_from(const data& in) NOEXCEPT
{
    constexpr auto start = sizeof(uint16_t);
    return from_little_array<uint32_t>(
        slice<start, start + sizeof(uint32_t)>(in));
}

uint32_t certificate::not_valid_after(const data& in) NOEXCEPT
{
    constexpr auto start = sizeof(uint16_t) + sizeof(uint32_t);
    return from_little_array<uint32_t>(
        slice<start, start + sizeof(uint32_t)>(in));
}

std::string certificate::encode_authority(const ec_xonly& authority) NOEXCEPT
{
    return encode_base58(insert_checksum<checked_size>(
    {
        authority_prefix,
        authority
    }));
}

bool certificate::decode_authority(ec_xonly& out,
    const std::string& text) NOEXCEPT
{
    data_array<checked_size> checked{};
    if (!decode_base58(checked, text) || !verify_checksum(checked) ||
        slice<zero, sizeof(uint16_t)>(checked) != authority_prefix)
        return false;

    out = slice<sizeof(uint16_t), authority_size>(checked);
    return true;
}

} // namespace noise
} // namespace network
} // namespace libbitcoin
