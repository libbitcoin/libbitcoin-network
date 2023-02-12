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
#include <bitcoin/network/config/endpoint.hpp>

#include <iostream>
#include <sstream>
#include <bitcoin/system.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/boost.hpp>
#include <bitcoin/network/config/authority.hpp>
#include <bitcoin/network/config/utilities.hpp>

namespace libbitcoin {
namespace network {
namespace config {

using namespace system;

// Contructors.
// ----------------------------------------------------------------------------

endpoint::endpoint() NOEXCEPT
  : endpoint({}, "localhost", {})
{
}

endpoint::endpoint(const std::string& uri) NOEXCEPT(false)
  : endpoint()
{
    std::stringstream(uri) >> *this;
}

endpoint::endpoint(const std::string& host, uint16_t port) NOEXCEPT
  : endpoint({}, host, port)
{
}

endpoint::endpoint(const std::string& scheme, const std::string& host,
    uint16_t port) NOEXCEPT
  : scheme_(scheme), host_(host), port_(port)
{
}

endpoint::endpoint(const asio::endpoint& uri) NOEXCEPT
  : endpoint(uri.address(), uri.port())
{
}

endpoint::endpoint(const asio::address& ip, uint16_t port) NOEXCEPT
  : endpoint(config::to_host(ip), port)
{
}

endpoint::endpoint(const config::authority& authority) NOEXCEPT
  : endpoint(authority.ip(), authority.port())
{
}

// Properties.
// ----------------------------------------------------------------------------

const std::string& endpoint::scheme() const NOEXCEPT
{
    return scheme_;
}

const std::string& endpoint::host() const NOEXCEPT
{
    return host_;
}

uint16_t endpoint::port() const NOEXCEPT
{
    return port_;
}

// Methods.
// ----------------------------------------------------------------------------

std::string endpoint::to_uri() const NOEXCEPT
{
    std::stringstream value{};
    value << *this;
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return value.str();
    BC_POP_WARNING()
}

endpoint endpoint::to_local() const NOEXCEPT
{
    const auto host = (host_ == "*" ? "localhost" : host_);
    return endpoint(scheme_, host, port_);
}

address endpoint::to_address() const NOEXCEPT
{
    try
    {
        return { to_uri() };
    }
    catch (std::exception)
    {
        return {};
    }
}

// Operators.
// ----------------------------------------------------------------------------

endpoint::operator bool() const NOEXCEPT
{
    return !scheme_.empty();
}

bool endpoint::operator==(const endpoint& other) const NOEXCEPT
{
    return host_ == other.host_
        && port_ == other.port_
        && scheme_ == other.scheme_;
}

bool endpoint::operator!=(const endpoint& other) const NOEXCEPT
{
    return !(*this == other);
}

std::istream& operator>>(std::istream& input,
    endpoint& argument) NOEXCEPT(false)
{
    std::string value{};
    input >> value;

    // C++11: use std::regex.
    // TODO: boost URI parser?
    // std::regex requires gcc 4.9, so we are using boost::regex for now.
    using namespace boost;
    static const regex regular
    {
        "^((tcp|udp|http|https|inproc):\\/\\/)?"
        "(\\[([0-9a-f:\\.]+)]|([^:]+))"
        "(:([1-9][0-9]{0,4}))?$"
    };

    sregex_iterator it{ value.begin(), value.end(), regular }, end{};
    if (it == end)
        throw istream_exception(value);

    const auto& token = *it;
    argument.scheme_ = token[2];
    argument.host_ = token[3];
    deserialize(argument.port_, token[7]);
    return input;
}

std::ostream& operator<<(std::ostream& output,
    const endpoint& argument) NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    output
        << (argument.scheme().empty() ? "" : argument.scheme() + "://")
        << (argument.host())
        << (is_zero(argument.port()) ? "" : ":" + serialize(argument.port()));
    BC_POP_WARNING()
    return output;
}

} // namespace config
} // namespace network
} // namespace libbitcoin
