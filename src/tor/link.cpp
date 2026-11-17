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
#include <bitcoin/network/tor/link.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tor/cell.hpp>
#include <bitcoin/network/tor/certificate.hpp>
#include <bitcoin/network/tor/constants.hpp>

// tor-spec 4 (negotiating and initializing channels)

namespace libbitcoin {
namespace network {
namespace tor {

using namespace system;

static bool contains(const std::vector<uint8_t>& list, uint8_t value) NOEXCEPT
{
    return std::find(list.cbegin(), list.cend(), value) != list.cend();
}

// Construct.
// ----------------------------------------------------------------------------

link::link(const hash_digest& certificate, const ip::address& relay,
    uint64_t time) NOEXCEPT
  : certificate_(certificate), relay_(relay), time_(time)
{
    data_chunk versions{};
    write::bytes::data sink(versions);
    for (const auto version: link_versions)
        sink.write_2_bytes_big_endian(version);

    sink.flush();
    output_ = encode({ zero, command::versions, versions }, true);
}

// Properties.
// ----------------------------------------------------------------------------

data_chunk& link::output() NOEXCEPT
{
    return output_;
}

data_chunk& link::remainder() NOEXCEPT
{
    return input_;
}

bool link::is_established() const NOEXCEPT
{
    return state_ == state::established;
}

bool link::is_failed() const NOEXCEPT
{
    return failure_ != failure::none;
}

link::failure link::reason() const NOEXCEPT
{
    return failure_;
}

uint16_t link::version() const NOEXCEPT
{
    return version_;
}

const short_hash& link::rsa_identity() const NOEXCEPT
{
    return rsa_identity_;
}

const ed25519::key& link::ed25519_identity() const NOEXCEPT
{
    return ed25519_identity_;
}

// Cells.
// ----------------------------------------------------------------------------

bool link::fail(failure value) NOEXCEPT
{
    failure_ = value;
    return false;
}

bool link::receive(const const_byte_span& data) NOEXCEPT
{
    if (is_failed())
        return false;

    input_.insert(input_.end(), data.begin(), data.end());

    // VERSIONS cell has narrow circuit ids (tor-spec 4.1).
    size_t start{};
    while (!is_established() && !is_failed())
    {
        cell value{};
        const auto available = const_byte_span{ input_ }.subspan(start);
        const auto size = decode(value, available, state_ == state::versions);
        if (is_zero(size))
            break;

        start += size;
        handle_cell(value);
    }

    input_.erase(input_.begin(), std::next(input_.begin(), start));
    return !is_failed();
}

// Responder sends VERSIONS, CERTS, AUTH_CHALLENGE, NETINFO, any padding.
bool link::handle_cell(const cell& value) NOEXCEPT
{
    const auto padding = (value.command == command::padding) ||
        (value.command == command::vpadding);

    switch (state_)
    {
        case state::versions:
        {
            if (value.command == command::versions)
                return handle_versions(value);

            break;
        }
        case state::certs:
        {
            if (padding)
                return true;

            if (value.command == command::certs)
                return handle_certs(value);

            break;
        }
        case state::netinfo:
        {
            if (padding)
                return true;

            if (!challenged_ && (value.command == command::auth_challenge))
            {
                challenged_ = true;
                return true;
            }

            if (value.command == command::netinfo)
                return handle_netinfo(value);

            break;
        }
        default:
        {
            break;
        }
    }

    return fail(failure::unexpected);
}

// Highest version offered by both (tor-spec 4.1).
bool link::handle_versions(const cell& value) NOEXCEPT
{
    const auto& payload = value.payload;
    if (is_odd(payload.size()))
        return fail(failure::malformed);

    read::bytes::copy source(payload);
    while (!source.is_exhausted())
    {
        const auto offered = source.read_2_bytes_big_endian();
        if (std::find(link_versions.cbegin(), link_versions.cend(), offered)
            != link_versions.cend())
            version_ = std::max(version_, offered);
    }

    if (is_zero(version_))
        return fail(failure::version);

    state_ = state::certs;
    return true;
}

// Responder chain (tor-spec 4.2): Ed25519 identity certifies signing key,
// signing key certifies TLS certificate digest, RSA identity certifies
// Ed25519 identity.
bool link::handle_certs(const cell& value) NOEXCEPT
{
    read::bytes::copy source(value.payload);
    const auto count = source.read_byte();
    std::vector<uint8_t> types{};
    std::array<data_chunk, add1(certificate_type::rsa_crosscert)> bodies{};
    for (size_t index{}; index < count; ++index)
    {
        const auto type = source.read_byte();
        const auto size = source.read_2_bytes_big_endian();
        auto body = source.read_bytes(size);
        if (!source)
            return fail(failure::malformed);

        if (contains(types, type))
            return fail(failure::certificate);

        types.push_back(type);
        if (type < bodies.size())
            bodies.at(type) = std::move(body);
    }

    if (!source.is_exhausted())
        return fail(failure::malformed);

    x509::certificate identity{};
    rsa::public_key rsa_key{};
    if (!x509::parse(identity, bodies.at(certificate_type::rsa_identity)) ||
        (identity.key_type != x509::key_type::rsa) ||
        !rsa::decode(rsa_key, identity.public_key))
        return fail(failure::certificate);

    certificate signing{};
    if (!parse(signing, bodies.at(certificate_type::identity_signing)) ||
        (signing.type != certificate_type::identity_signing) ||
        (signing.key_type != key_type::ed25519) || !signing.has_signer ||
        !is_signed_by(signing, signing.signer))
        return fail(failure::certificate);

    certificate binding{};
    if (!parse(binding, bodies.at(certificate_type::signing_link)) ||
        (binding.type != certificate_type::signing_link) ||
        (binding.key_type != key_type::sha256_x509) ||
        (binding.key != certificate_) || !is_signed_by(binding, signing.key))
        return fail(failure::certificate);

    crosscert cross{};
    if (!parse(cross, bodies.at(certificate_type::rsa_crosscert)) ||
        (cross.key != signing.signer) || !is_signed_by(cross, rsa_key))
        return fail(failure::certificate);

    if (is_expired(signing.expiration, time_) ||
        is_expired(binding.expiration, time_) ||
        is_expired(cross.expiration, time_))
        return fail(failure::expired);

    rsa_identity_ = sha1_hash(identity.public_key);
    ed25519_identity_ = signing.signer;
    state_ = state::netinfo;
    return true;
}

// Responder time and addresses unused (tor-spec 4.5).
bool link::handle_netinfo(const cell& value) NOEXCEPT
{
    read::bytes::copy source(value.payload);
    source.read_4_bytes_big_endian();
    source.read_byte();
    source.skip_bytes(source.read_byte());
    const auto addresses = source.read_byte();
    for (size_t index{}; index < addresses; ++index)
    {
        source.read_byte();
        source.skip_bytes(source.read_byte());
    }

    if (!source)
        return fail(failure::malformed);

    send_netinfo();
    state_ = state::established;
    return true;
}

// Client sends no time and no own addresses (tor-spec 4.5).
void link::send_netinfo() NOEXCEPT
{
    data_chunk payload{};
    write::bytes::data sink(payload);
    sink.write_4_bytes_big_endian(0);
    if (relay_.is_v4())
    {
        const auto& bytes = relay_.to_v4().to_bytes();
        sink.write_byte(address_ipv4);
        sink.write_byte(possible_narrow_cast<uint8_t>(bytes.size()));
        sink.write_bytes(bytes);
    }
    else
    {
        const auto& bytes = relay_.to_v6().to_bytes();
        sink.write_byte(address_ipv6);
        sink.write_byte(possible_narrow_cast<uint8_t>(bytes.size()));
        sink.write_bytes(bytes);
    }

    sink.write_byte(0);
    sink.flush();

    const auto netinfo = encode({ zero, command::netinfo, payload });
    output_.insert(output_.end(), netinfo.begin(), netinfo.end());
}

} // namespace tor
} // namespace network
} // namespace libbitcoin
