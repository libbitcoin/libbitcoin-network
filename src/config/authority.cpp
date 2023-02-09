/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/config/authority.hpp>

#include <iostream>
#include <sstream>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/network/messages/messages.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace config {

using namespace system;

// Contructors.
// ----------------------------------------------------------------------------

authority::authority() NOEXCEPT
  : authority(asio::ipv6{}, {})
{
}

authority::authority(const std::string& authority) NOEXCEPT(false)
  : ip_{}, port_{}, cidr_{}
{
    std::stringstream(authority) >> *this;
}

// This allows unusable CIDR values (ok).
authority::authority(const asio::address& ip, uint16_t port,
    uint8_t cidr) NOEXCEPT
  : ip_(ip), port_(port), cidr_(cidr)
{
}

authority::authority(const asio::endpoint& endpoint) NOEXCEPT
  : authority(endpoint.address(), endpoint.port())
{
}

authority::authority(const config::address& address) NOEXCEPT
  : authority(address.to_ip(), address.port())
{
}

// Properties.
// ----------------------------------------------------------------------------

const asio::address& authority::ip() const NOEXCEPT
{
    return ip_;
}

uint16_t authority::port() const NOEXCEPT
{
    return port_;
}

uint8_t authority::cidr() const NOEXCEPT
{
    return cidr_;
}

// Methods.
// ----------------------------------------------------------------------------

std::string authority::to_host() const NOEXCEPT
{
    return config::to_host(ip_);
}

std::string authority::to_literal() const NOEXCEPT
{
    return config::to_literal(ip_);
}

std::string authority::to_string() const NOEXCEPT
{
    std::stringstream value{};
    value << *this;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return value.str();
    BC_POP_WARNING()
}

messages::address_item authority::to_address_item() const NOEXCEPT
{
    return to_address_item({}, messages::service::node_none);
}

messages::address_item authority::to_address_item(uint32_t timestamp,
    uint64_t services) const NOEXCEPT
{
    return { timestamp, services, to_ip_address(), port() };
}

messages::ip_address authority::to_ip_address() const NOEXCEPT
{
    return config::to_address(ip_);
}

// Operators.
// ----------------------------------------------------------------------------

authority::operator bool() const NOEXCEPT
{
    return !is_zero(port_) && !ip_.is_unspecified();
}

bool authority::operator==(const authority& other) const NOEXCEPT
{
    // both non-zero ports must match.
    if ((!is_zero(port()) || !is_zero(other.port())) && port() != other.port())
        return false;

    // both non-zero cidrs must match.
    if ((!is_zero(cidr()) && !is_zero(other.cidr())) && cidr() != other.cidr())
        return false;

    // matching cidrs, match ips only.
    if (cidr() == other.cidr())
        return ip() == other.ip();

    // match host to network.
    return is_zero(cidr()) ?
        config::is_member(ip(), other.ip(), other.cidr()) :
        config::is_member(other.ip(), ip(), cidr());
}

bool authority::operator!=(const authority& other) const NOEXCEPT
{
    return !(*this == other);
}

std::istream& operator>>(std::istream& input,
    authority& argument) NOEXCEPT(false)
{
    std::string value{};
    input >> value;

    // C++11: use std::regex.
    using namespace boost;
    static const regex regular
    {
        // This excludes /0, so cidr_(0) implies address (vs. network).
        "^(([0-9\\.]+)|\\[([0-9a-f:\\.]+)])"
        "(:([1-9][0-9]{0,4}))?"
        "(/([1-9][0-9]{0,2}))?$"
    };

    sregex_iterator it{ value.begin(), value.end(), regular }, end{};
    if (it == end)
        throw istream_exception(value);

    const auto& token = *it;
    const auto host = is_zero(token[3].length()) ? token[2] : token[3];
    deserialize(argument.port_, token[5]);
    deserialize(argument.cidr_, token[7]);
    argument.ip_ = from_host(host);

    // This allows /33-64 CIDR values for mapped ipv4 addresses (unusable, ok).
    if ((argument.ip_.is_v4() && argument.cidr_ > bits<uint32_t>) ||
        (argument.ip_.is_v6() && argument.cidr_ > bits<uint64_t>))
    {
        throw istream_exception(value);
    }

    return input;
}

// This allows unusable CIDR values (ok).
std::ostream& operator<<(std::ostream& output,
    const authority& argument) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    output
        << argument.to_literal()
        << (!is_zero(argument.port()) ? ":" + serialize(argument.port()) : "")
        << (!is_zero(argument.cidr()) ? "/" + serialize(argument.cidr()) : "");
    BC_POP_WARNING()
    return output;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
