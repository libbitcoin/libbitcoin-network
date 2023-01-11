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
#ifndef LIBBITCOIN_NETWORK_CONFIG_ENDPOINT_HPP
#define LIBBITCOIN_NETWORK_CONFIG_ENDPOINT_HPP

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/config/authority.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Serialization helper for a network endpoint in URI format.
/// This is a container for a {scheme, host, port} tuple.
class BCT_API endpoint
{
public:
    DEFAULT5(endpoint);

    typedef std::shared_ptr<endpoint> ptr;

    endpoint() NOEXCEPT;

    /// The scheme and port may be undefined, in which case the port is
    /// reported as zero and the scheme is reported as an empty string.
    /// The value is of the form: [scheme://]host[:port]
    endpoint(const std::string& uri) NOEXCEPT(false);
    endpoint(const authority& authority) NOEXCEPT;

    /// host may be host name or ip address.
    endpoint(const std::string& host, uint16_t port) NOEXCEPT;
    endpoint(const std::string& scheme, const std::string& host,
        uint16_t port) NOEXCEPT;

    endpoint(const asio::endpoint& uri) NOEXCEPT;
    endpoint(const asio::address& ip, uint16_t port) NOEXCEPT;

    /// True if the endpoint is initialized.
    operator bool() const NOEXCEPT;

    /// The scheme of the endpoint or empty string.
    const std::string& scheme() const NOEXCEPT;

    /// The host name or ip address of the endpoint.
    const std::string& host() const NOEXCEPT;

    /// The tcp port of the endpoint.
    uint16_t port() const NOEXCEPT;

    /// An empty scheme and/or empty port is omitted.
    /// The endpoint is of the form: [scheme://]host[:port]
    std::string to_string() const NOEXCEPT;

    /// Return a new endpoint that replaces host instances of "*" with
    /// "localhost". This is intended for clients that wish to connect
    /// to a service that has been configured to bind to all interfaces.
    /// The endpoint is of the form: [scheme://]host[:port]
    endpoint to_local() const NOEXCEPT;

    bool operator==(const endpoint& other) const NOEXCEPT;

    friend std::istream& operator>>(std::istream& input,
        endpoint& argument) NOEXCEPT(false);
    friend std::ostream& operator<<(std::ostream& output,
        const endpoint& argument) NOEXCEPT;

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
        return std::hash<std::string>{}(value.to_string());
    }
};
} // namespace std

#endif
