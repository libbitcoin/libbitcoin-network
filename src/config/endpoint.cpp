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

#include <bitcoin/system.hpp>
#include <bitcoin/network/config/address.hpp>
#include <bitcoin/network/config/utilities.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace config {

// protected
address endpoint::to_address() const NOEXCEPT
{
    try
    {
        // Throws if textual authority does not parse to IP address.
        return { to_authority() };
    }
    catch (const std::exception&)
    {
        return {};
    }
}

// protected
messages::address_item endpoint::to_address_item() const NOEXCEPT
{
    return to_address();
}

endpoint::operator const address() const NOEXCEPT
{
    return to_address();
}

endpoint::operator const authority() const NOEXCEPT
{
    return authority{ to_address() };
}

bool endpoint::operator==(const messages::address_item& other) const NOEXCEPT
{
    // Will match default address_item if to_address_item() returns default.
    return to_address_item() == other;
}

bool endpoint::operator!=(const messages::address_item& other) const NOEXCEPT
{
    return !(*this == other);
}

} // namespace config
} // namespace network
} // namespace libbitcoin
