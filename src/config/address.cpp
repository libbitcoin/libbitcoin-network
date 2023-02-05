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
#include <bitcoin/network/config/address.hpp>

#include <iostream>
#include <sstream>
#include <bitcoin/network/config/authority.hpp>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace config {

address::address() NOEXCEPT
{
}

address::address(const std::string& host) NOEXCEPT(false)
{
    std::stringstream(host) >> *this;
}

address::address(const messages::address_item& host) NOEXCEPT
  : address_(host)
{
}

address::operator bool() const NOEXCEPT
{
    return !is_zero(address_.port) &&
        address_.ip != messages::unspecified_ip_address;
}

const messages::address_item& address::item() const NOEXCEPT
{
    return address_;
}

std::string address::to_string() const NOEXCEPT
{
    std::stringstream value{};
    value << *this;

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return value.str();
    BC_POP_WARNING()
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
    const authority host{ tokens.at(0) };

    // Assign with default timestamp (0) and services (services::node_none).
    argument.address_ = host.to_address_item();

    if (tokens.size() > 1)
    {
        if (!deserialize(argument.address_.timestamp, tokens.at(1)))
            throw istream_exception(tokens.at(1));

        if (tokens.size() > 2)
        {
            if (!deserialize(argument.address_.services, tokens.at(2)))
                throw istream_exception(tokens.at(2));
        }
    }

    return input;
}

std::ostream& operator<<(std::ostream& output,
    const address& argument) NOEXCEPT
{
    const authority host{ argument.address_ };

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    output << host
        << "/" << argument.address_.timestamp
        << "/" << argument.address_.services;
    BC_POP_WARNING()
    return output;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
