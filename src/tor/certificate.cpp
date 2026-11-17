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
#include <bitcoin/network/tor/certificate.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tor/constants.hpp>

// cert-spec 2 (ed25519 certificates), 2.3 (rsa cross-certificate)

namespace libbitcoin {
namespace network {
namespace tor {

using namespace system;

constexpr uint64_t seconds_per_hour = 3600;

// Cross-certificate signs digest of this, key and expiration (cert-spec 2.3).
constexpr auto crosscert_prefix = to_array(
    "Tor TLS RSA/Ed25519 cross-certificate");

bool parse(certificate& out, const_byte_span data) NOEXCEPT
{
    read::bytes::copy source({ data.begin(), data.end() });
    certificate value{};
    const auto version = source.read_byte();
    value.type = source.read_byte();
    value.expiration = source.read_4_bytes_big_endian();
    value.key_type = source.read_byte();
    value.key = source.read_forward<ed25519::key_size>();

    // Some relays declare link certificate digest as a key (tor 40124).
    if ((value.type == certificate_type::signing_link) &&
        (value.key_type == key_type::ed25519))
        value.key_type = key_type::sha256_x509;

    const auto extensions = source.read_byte();
    for (size_t index{}; index < extensions; ++index)
    {
        const auto size = source.read_2_bytes_big_endian();
        const auto type = source.read_byte();
        const auto flags = source.read_byte();
        const auto body = source.read_bytes(size);
        if (type == signed_with_ed25519)
        {
            if (value.has_signer || (body.size() != ed25519::key_size))
                return false;

            std::copy(body.begin(), body.end(), value.signer.begin());
            value.has_signer = true;
        }
        else if (is_nonzero(bit_and(flags, affects_validation)))
        {
            return false;
        }
    }

    value.signature = source.read_forward<ed25519::signature_size>();
    if (!source || !source.is_exhausted() || (version != certificate_version))
        return false;

    const auto end = std::prev(data.end(), ed25519::signature_size);
    value.signed_data.assign(data.begin(), end);
    out = std::move(value);
    return true;
}

bool parse(crosscert& out, const_byte_span data) NOEXCEPT
{
    read::bytes::copy source({ data.begin(), data.end() });
    crosscert value{};
    value.key = source.read_forward<ed25519::key_size>();
    value.expiration = source.read_4_bytes_big_endian();
    const auto size = source.read_byte();
    value.signature = source.read_bytes(size);
    if (!source || !source.is_exhausted())
        return false;

    const auto signed_size = ed25519::key_size + sizeof(value.expiration);
    const auto end = std::next(data.begin(), signed_size);
    value.signed_data.assign(data.begin(), end);
    out = std::move(value);
    return true;
}

bool is_signed_by(const certificate& value,
    const ed25519::key& key) NOEXCEPT
{
    return ed25519::verify(value.signature, key, value.signed_data);
}

bool is_signed_by(const crosscert& value,
    const rsa::public_key& key) NOEXCEPT
{
    accumulator<sha256> hasher{};
    hasher.write(crosscert_prefix);
    hasher.write(value.signed_data);
    return rsa::verify_pkcs1(key, value.signature, hasher.flush());
}

bool is_expired(uint32_t expiration, uint64_t time) NOEXCEPT
{
    return time >= (expiration * seconds_per_hour);
}

} // namespace tor
} // namespace network
} // namespace libbitcoin
