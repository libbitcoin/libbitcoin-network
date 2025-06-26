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
#ifndef LIBBITCOIN_NETWORK_CONFIG_ENDPOINT_HPP
#define LIBBITCOIN_NETWORK_CONFIG_ENDPOINT_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/config/address.hpp>
#include <bitcoin/network/config/authority.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Add message address types to base endpoint configuration class.
class BCT_API endpoint
  : public system::config::endpoint
{
public:
    typedef std::shared_ptr<endpoint> ptr;

    DEFAULT_COPY_MOVE_DESTRUCT(endpoint);

    using system::config::endpoint::endpoint;

    /// Operators.
    /// -----------------------------------------------------------------------

    /// If endpoint is a DNS name (not numeric) default address is returned.
    operator address() const NOEXCEPT;
    operator authority() const NOEXCEPT;

    /// Equality considers all properties (scheme, host, port).
    /// Non-numeric and invalid endpoints will match the default address_item.
    bool operator==(const messages::address_item& other) const NOEXCEPT;
    bool operator!=(const messages::address_item& other) const NOEXCEPT;

protected:
    address to_address() const NOEXCEPT;
    messages::address_item to_address_item() const NOEXCEPT;
};

typedef std::vector<endpoint> endpoints;

} // namespace config
} // namespace network
} // namespace libbitcoin

namespace std
{
template<>
struct hash<bc::network::config::endpoint>
{
    size_t operator()(const bc::network::config::endpoint& value) const NOEXCEPT
    {
        return std::hash<std::string>{}(value.to_uri());
    }
};
} // namespace std

#endif
