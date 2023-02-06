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

// string conversion.

address::address(const std::string& host) NOEXCEPT(false)
{
    std::stringstream(host) >> *this;
}

// message conversion.

address::address(messages::address_item&& item) NOEXCEPT
  : address_(system::to_shared(std::move(item)))
{
}

address::address(const messages::address_item& item) NOEXCEPT
  : address_(system::to_shared(item))
{
}

address::address(const messages::address_item::cptr& item) NOEXCEPT
  : address_(item ? item : system::to_shared<messages::address_item>())
{
}

// Properties.
// ----------------------------------------------------------------------------

const messages::address_item& address::item() const NOEXCEPT
{
    return *address_;
}

const messages::address_item::cptr& address::item_ptr() const NOEXCEPT
{
    return address_;
}

// Methods.
// ----------------------------------------------------------------------------

std::string address::to_string() const NOEXCEPT
{
    std::stringstream value{};
    value << *this;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return value.str();
    BC_POP_WARNING()
}

std::string address::to_host() const NOEXCEPT
{
    return to_ip_host(to_address(address_->ip));
}

uint16_t address::port() const NOEXCEPT
{
    return address_->port;
}

// Operators.
// ----------------------------------------------------------------------------

address::operator bool() const NOEXCEPT
{
    return is_valid(*address_);
}

bool address::operator==(const address& other) const NOEXCEPT
{
    return address_ == other.address_;
}

bool address::operator!=(const address& other) const NOEXCEPT
{
    return !(*this == other);
}

std::istream& operator>>(std::istream& input,
    address& argument) NOEXCEPT(false)
{
    std::string line{};
    input >> line;

    using namespace system;
    const auto tokens = split(line, "/", true, false);
    if (is_limited(tokens.size(), 1, 3))
        throw istream_exception(line);

    // Throws istream_exception if parse fails.
    // Sets default timestamp (0) and services (services::node_none).
    auto item = authority{ tokens.at(0) }.to_address_item();

    if (tokens.size() > 1)
    {
        if (!deserialize(item.timestamp, tokens.at(1)))
            throw istream_exception(tokens.at(1));

        if (tokens.size() > 2)
        {
            if (!deserialize(item.services, tokens.at(2)))
                throw istream_exception(tokens.at(2));
        }
    }

    argument.address_ = to_shared(std::move(item));
    return input;
}

std::ostream& operator<<(std::ostream& output,
    const address& argument) NOEXCEPT
{
    const authority host{ argument };

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    output << host
        << "/" << argument.address_->timestamp
        << "/" << argument.address_->services;
    BC_POP_WARNING()
    return output;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
