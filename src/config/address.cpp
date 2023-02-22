/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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

#include <iostream>
#include <sstream>
#include <utility>
#include <bitcoin/network/config/authority.hpp>
#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace config {

// Constructors.
// ----------------------------------------------------------------------------

address::address() NOEXCEPT
  : address(system::to_shared<messages::address_item>())
{
}

address::address(const std::string& host) THROWS
  : address()
{
    std::stringstream(host) >> *this;
}

address::address(messages::address_item&& item) NOEXCEPT
  : address(system::to_shared(std::move(item)))
{
}

address::address(const messages::address_item& item) NOEXCEPT
  : address(system::to_shared(item))
{
}

address::address(const messages::address_item::cptr& message) NOEXCEPT
  : address_(message ? message : system::to_shared<messages::address_item>())
{
}

// Methods.
// ----------------------------------------------------------------------------

asio::address address::to_ip() const NOEXCEPT
{
    return config::denormalize(from_address(address_->ip));
}

std::string address::to_host() const NOEXCEPT
{
    return config::to_host(to_ip());
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
    return config::is_v4(address_->ip);
}

bool address::is_v6() const NOEXCEPT
{
    return !is_v4();
}

const messages::ip_address& address::ip() const NOEXCEPT
{
    return address_->ip;
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

// Operators.
// ----------------------------------------------------------------------------

address::operator const messages::address_item& () const NOEXCEPT
{
    return *address_;
}

address::operator const messages::address_item::cptr& () const NOEXCEPT
{
    return address_;
}

address::operator bool() const NOEXCEPT
{
    return messages::is_specified(*address_);
}

bool address::operator==(const address& other) const NOEXCEPT
{
    return (address_->ip == other.address_->ip)
        && ((address_->port == other.address_->port) ||
            (is_zero(address_->port) || is_zero(other.address_->port)));
}

bool address::operator!=(const address& other) const NOEXCEPT
{
    return !(*this == other);
}

bool address::operator==(const messages::address_item& other) const NOEXCEPT
{
    return (address_->ip == other.ip)
        && ((address_->port == other.port) ||
            (is_zero(address_->port) || is_zero(other.port)));
}

bool address::operator!=(const messages::address_item& other) const NOEXCEPT
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
    auto item = authority{ tokens.at(0) }.to_address_item();

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
    const authority host{ argument };
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    output
        << host
        << "/" << argument.address_->timestamp
        << "/" << argument.address_->services;
    BC_POP_WARNING()
    return output;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
