/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include <bitcoin/network/config/endpoint.hpp>

#include <algorithm>
#include <bitcoin/network/config/address.hpp>
#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace config {

endpoint::endpoint(const address& address) NOEXCEPT
  : endpoint(address.to_ip(), address.port())
{
}

bool endpoint::is_address() const NOEXCEPT
{
    // Serialize to address, cast to bool, true if not default (address).
    return to_address();
}

// protected
address endpoint::to_address() const NOEXCEPT
{
    try
    {
        // Throws if textual authority does not parse to IP address.
        return { to_string() };
    }
    catch (const std::exception&)
    {
        return {};
    }
}

// protected
messages::peer::address_item endpoint::to_address_item() const NOEXCEPT
{
    return to_address();
}

endpoint::operator address() const NOEXCEPT
{
    return to_address();
}

endpoint::operator authority() const NOEXCEPT
{
    return authority{ to_address() };
}

bool endpoint::operator==(const messages::peer::address_item& other) const NOEXCEPT
{
    // Will match default address_item if to_address_item() returns default.
    return to_address_item() == other;
}

bool endpoint::operator!=(const messages::peer::address_item& other) const NOEXCEPT
{
    return !(*this == other);
}

// public utility
system::string_list to_host_names(const endpoints& hosts,
    uint16_t default_port) NOEXCEPT
{
    system::string_list out{};
    out.resize(hosts.size());
    std::ranges::transform(hosts, out.begin(), [=](const auto& value) NOEXCEPT
    {
        return value.to_lower(default_port);
    });

    return out;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
