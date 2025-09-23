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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_HEADING_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_HEADING_HPP

#include <map>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

struct BCT_API heading
{
    /// Ordered multimap ensures consistent output order.
    using fields = std::multimap<std::string, std::string>;

    static const std::string space;
    static const std::string separator;
    static const std::string separators;
    static const std::string line;
    static const std::string terminal;

    /// Canonical serialization buffer size (includes 1 OWS SP).
    static size_t fields_size(const fields& fields) NOEXCEPT;
    static fields to_fields(system::reader& source) NOEXCEPT;
    static void from_fields(const fields& fields,
        system::writer& sink) NOEXCEPT;
};

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
