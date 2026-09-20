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
#include <bitcoin/network/config/address.hpp>

#include <sstream>
#include <bitcoin/network/config/authority.hpp>
#include <bitcoin/network/config/endpoint.hpp>
#include <bitcoin/network/config/utilities.hpp>

namespace libbitcoin {
namespace network {
namespace config {

using namespace system;
using address_t = messages::peer::address_t;
using address_item = messages::peer::address_item;
using torv3_t = messages::peer::torv3_t;
using i2p_t = messages::peer::i2p_t;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Host names.
// ----------------------------------------------------------------------------
// Tor and i2p addresses are named, all other networks are ip addresses.

constexpr auto onion_suffix = ".onion";
constexpr auto i2p_suffix = ".b32.i2p";
constexpr uint8_t onion_version = 3;
constexpr auto onion_prefix = to_array(".onion checksum");
constexpr auto onion_size = 56_size;
constexpr auto i2p_size = 52_size;

// The two byte checksum of the tor public key and address version.
static data_array<2> to_checksum(const torv3_t& value) NOEXCEPT
{
    const auto preimage = build_array<48>(
    {
        onion_prefix, value.value, to_array(onion_version)
    });

    const auto digest = sha3_256::simple_hash(preimage);
    return { digest.front(), digest.at(1) };
}

static std::string to_onion(const torv3_t& value) NOEXCEPT
{
    const auto payload = build_array<35>(
    {
        value.value, to_checksum(value), to_array(onion_version)
    });

    return ascii_to_lower(encode_base32(payload)) + onion_suffix;
}

static std::string to_i2p(const i2p_t& value) NOEXCEPT
{
    // Thirty two bytes encode to fifty two characters and four pads.
    return ascii_to_lower(encode_base32(value.value).substr(zero, i2p_size)) +
        i2p_suffix;
}

static bool from_onion(address_t& out, const std::string& name) NOEXCEPT
{
    data_chunk payload{};
    if (name.size() != onion_size || !decode_base32(payload, name) ||
        payload.size() != 35 || payload.back() != onion_version)
        return false;

    torv3_t value{};
    std::copy_n(payload.cbegin(), value.value.size(), value.value.begin());
    const auto checksum = to_checksum(value);
    const auto start = std::next(payload.cbegin(), value.value.size());
    if (!std::equal(checksum.begin(), checksum.end(), start))
        return false;

    out = value;
    return true;
}

static bool from_i2p(address_t& out, const std::string& name) NOEXCEPT
{
    data_chunk payload{};
    if (name.size() != i2p_size || !decode_base32(payload, name) ||
        payload.size() != 32)
        return false;

    i2p_t value{};
    std::copy(payload.cbegin(), payload.cend(), value.value.begin());
    out = value;
    return true;
}

// False if the host is not a tor or i2p name (may still be an authority).
static bool from_name(address_item& item, const std::string& token) NOEXCEPT
{
    const auto colon = token.rfind(':');
    const auto host = token.substr(zero, colon);

    address_t address{};
    if (host.ends_with(onion_suffix))
    {
        if (!from_onion(address, host.substr(zero, host.size() -
            std::char_traits<char>::length(onion_suffix))))
            return false;
    }
    else if (host.ends_with(i2p_suffix))
    {
        if (!from_i2p(address, host.substr(zero, host.size() -
            std::char_traits<char>::length(i2p_suffix))))
            return false;
    }
    else
    {
        return false;
    }

    uint16_t port{};
    if (colon != std::string::npos &&
        !deserialize(port, token.substr(add1(colon))))
        return false;

    item =
    {
        messages::peer::unspecified_timestamp,
        messages::peer::service::node_none, address, port
    };

    return true;
}

// True if the address has no host name (is an ip address).
static bool is_ip(const address_t& address) NOEXCEPT
{
    return !std::holds_alternative<torv3_t>(address)
        && !std::holds_alternative<i2p_t>(address);
}

// Constructors.
// ----------------------------------------------------------------------------

address::address() NOEXCEPT
  : address(system::to_shared<messages::peer::address_item>())
{
}

address::address(const std::string& host) THROWS
  : address()
{
    std::stringstream(host) >> *this;
}

address::address(messages::peer::address_item&& item) NOEXCEPT
  : address(system::to_shared(std::move(item)))
{
}

address::address(const messages::peer::address_item& item) NOEXCEPT
  : address(system::to_shared(item))
{
}

address::address(const messages::peer::address_item::cptr& message) NOEXCEPT
  : address_(message ? message : system::to_shared<messages::peer::address_item>())
{
}

address::address(const asio::endpoint& uri) NOEXCEPT
  : address(endpoint{ uri })
{
}

// Methods.
// ----------------------------------------------------------------------------

asio::address address::to_ip() const NOEXCEPT
{
    return system::config::denormalize(from_address(ip()));
}

std::string address::to_host() const NOEXCEPT
{
    if (const auto value = std::get_if<torv3_t>(&address_->address))
        return to_onion(*value);

    if (const auto value = std::get_if<i2p_t>(&address_->address))
        return to_i2p(*value);

    return system::config::to_host(to_ip());
}

std::string address::to_string() const NOEXCEPT
{
    std::stringstream value{};
    value << *this;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return value.str();
    BC_POP_WARNING()
}

// Properties.
// ----------------------------------------------------------------------------

bool address::is_v4() const NOEXCEPT
{
    return messages::peer::is_v4(address_->address);
}

bool address::is_v6() const NOEXCEPT
{
    return messages::peer::is_v6(address_->address);
}

messages::peer::address_item address::to_address_item(uint32_t timestamp,
    uint64_t services) const NOEXCEPT
{
    return { timestamp, services, address_->address, port() };
}

bool address::is_named() const NOEXCEPT
{
    return messages::peer::is_named(address_->address);
}

const messages::peer::ip_address& address::ip() const NOEXCEPT
{
    return messages::peer::to_ip_address(address_->address);
}

uint16_t address::port() const NOEXCEPT
{
    return address_->port;
}

uint32_t address::timestamp() const NOEXCEPT
{
    return address_->timestamp;
}

uint64_t address::services() const NOEXCEPT
{
    return address_->services;
}

bool address::is_advertised(messages::peer::service service) const NOEXCEPT
{
    using namespace system;
    return get_right(address_->services, right_zeros<uint64_t>(service));
}

// Operators.
// ----------------------------------------------------------------------------

address::operator const messages::peer::address_item& () const NOEXCEPT
{
    return *address_;
}

address::operator const messages::peer::address_item::cptr& () const NOEXCEPT
{
    return address_;
}

address::operator bool() const NOEXCEPT
{
    return messages::peer::is_specified(*address_);
}

bool address::operator==(const address& other) const NOEXCEPT
{
    return (address_->address == other.address_->address)
        && ((address_->port == other.address_->port) ||
            (is_zero(address_->port) || is_zero(other.address_->port)));
}

bool address::operator!=(const address& other) const NOEXCEPT
{
    return !(*this == other);
}

bool address::operator==(const messages::peer::address_item& other) const NOEXCEPT
{
    return (address_->address == other.address)
        && ((address_->port == other.port) ||
            (is_zero(address_->port) || is_zero(other.port)));
}

bool address::operator!=(const messages::peer::address_item& other) const NOEXCEPT
{
    return !(*this == other);
}

std::istream& operator>>(std::istream& input,
    address& argument) THROWS
{
    std::string line{};
    input >> line;

    using namespace system;
    const auto tokens = split(line, "/", true, false);
    if (is_limited(tokens.size(), 1, 3))
        throw istream_exception(line);

    // Throws istream_exception if parse fails.
    // Sets default timestamp (0) and services (services::node_none).
    // IPv4 addresses are converted to IPv6-mapped for message encoding.
    messages::peer::address_item item{};
    if (!from_name(item, tokens.at(0)))
        item = authority{ tokens.at(0) }.to_address_item();

    if (tokens.size() > 1)
        if (!deserialize(item.timestamp, tokens.at(1)))
            throw istream_exception(tokens.at(1));

    if (tokens.size() > 2)
        if (!deserialize(item.services, tokens.at(2)))
            throw istream_exception(tokens.at(2));

    argument.address_ = to_shared(std::move(item));
    return input;
}

std::ostream& operator<<(std::ostream& output,
    const address& argument) NOEXCEPT
{
    if (is_ip(argument.address_->address))
        output << authority{ argument };
    else
        output << argument.to_host() << (!is_zero(argument.port()) ?
            ":" + serialize(argument.port()) : "");

    output
        << "/" << argument.address_->timestamp
        << "/" << argument.address_->services;
    return output;
}

BC_POP_WARNING()

} // namespace config
} // namespace network
} // namespace libbitcoin
