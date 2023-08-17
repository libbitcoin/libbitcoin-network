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
#ifndef LIBBITCOIN_NETWORK_CONFIG_ENDPOINT_HPP
#define LIBBITCOIN_NETWORK_CONFIG_ENDPOINT_HPP

#include <iostream>
#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/address.hpp>
#include <bitcoin/network/config/authority.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace config {
    
/// Container for a [scheme, host, port] tuple.
/// IPv6 URIs encoded with literal host (en.wikipedia.org/wiki/IPv6_address).
/// Provided for serialization of network endpoints in URI format.
class BCT_API endpoint
{
public:
    typedef std::shared_ptr<endpoint> ptr;

    DEFAULT_COPY_MOVE_DESTRUCT(endpoint);

    endpoint() NOEXCEPT;

    /// The scheme and port may be undefined, in which case the port is
    /// reported as zero and the scheme is reported as an empty string.
    /// The value is of the form: [scheme://]host[:port] (dns name or ip).
    endpoint(const std::string& uri) THROWS;
    endpoint(const std::string& host, uint16_t port) NOEXCEPT;
    endpoint(const std::string& scheme, const std::string& host,
        uint16_t port) NOEXCEPT;
    endpoint(const asio::endpoint& uri) NOEXCEPT;
    endpoint(const asio::address& ip, uint16_t port) NOEXCEPT;
    endpoint(const config::authority& authority) NOEXCEPT;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// The scheme of the endpoint or empty string.
    const std::string& scheme() const NOEXCEPT;

    /// The host name or ip address of the endpoint.
    const std::string& host() const NOEXCEPT;

    /// The tcp port of the endpoint.
    uint16_t port() const NOEXCEPT;

    /// Methods.
    /// -----------------------------------------------------------------------

    /// An empty scheme and/or empty (zero) port is omitted.
    /// The endpoint is of the form: [scheme://]host[:port]
    std::string to_uri() const NOEXCEPT;

    /// Return a new endpoint that replaces host instances of "*" with
    /// "localhost". This is intended for clients that wish to connect
    /// to a service that has been configured to bind to all interfaces.
    endpoint to_local() const NOEXCEPT;

    /// IP address object if host is numeric, otherwise unspecified.
    address to_address() const NOEXCEPT;

    /// Operators.
    /// -----------------------------------------------------------------------

    /// Cast to IP address object if host is numeric, otherwise unspecified.
    operator const address() const NOEXCEPT;

    /// False if the endpoint is not initialized.
    operator bool() const NOEXCEPT;

    /// Equality considers all properties (scheme, host, port).
    /// Non-numeric and invalid endpoints will match the default address_item.
    bool operator==(const endpoint& other) const NOEXCEPT;
    bool operator!=(const endpoint& other) const NOEXCEPT;
    bool operator==(const messages::address_item& other) const NOEXCEPT;
    bool operator!=(const messages::address_item& other) const NOEXCEPT;

    friend std::istream& operator>>(std::istream& input,
        endpoint& argument) THROWS;
    friend std::ostream& operator<<(std::ostream& output,
        const endpoint& argument) NOEXCEPT;

protected:
    std::string to_authority() const NOEXCEPT;
    messages::address_item to_address_item() const NOEXCEPT;

private:
    // These are not thread safe.
    std::string scheme_;
    std::string host_;
    uint16_t port_;
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
