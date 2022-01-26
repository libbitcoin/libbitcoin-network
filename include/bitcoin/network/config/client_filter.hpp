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

// Sponsored in part by Digital Contract Design, LLC

#ifndef LIBBITCOIN_NETWORK_CONFIG_CLIENT_FILTER_HPP
#define LIBBITCOIN_NETWORK_CONFIG_CLIENT_FILTER_HPP

#include <iostream>
#include <memory>
#include <string>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace config {

/// Serialization helper for a client filter.
class BC_API client_filter
{
public:
    typedef std::shared_ptr<client_filter> ptr;

    client_filter();
    client_filter(const client_filter& other);
    client_filter(const std::string& hexcode);
    client_filter(const messages::client_filter& value);

    client_filter& operator=(const client_filter& other);
    client_filter& operator=(messages::client_filter&& other);

    bool operator==(const client_filter& other) const;

    operator const messages::client_filter&() const;

    std::string to_string() const;

    friend std::istream& operator>>(std::istream& input,
        client_filter& argument);
    friend std::ostream& operator<<(std::ostream& output,
        const client_filter& argument);

private:
    messages::client_filter value_;
};

} // namespace config
} // namespace network
} // namespace libbitcoin

#endif
