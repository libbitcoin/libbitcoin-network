/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include <bitcoin/network/privacy/commands.hpp>

#include <algorithm>
#include <string>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

using namespace system;

// bip324
// The short identifier is the array index plus one (1 to 28).
static const std_array<std::string, 28> commands
{
    "addr",         // 1
    "block",        // 2
    "blocktxn",     // 3
    "cmpctblock",   // 4
    "feefilter",    // 5
    "filteradd",    // 6
    "filterclear",  // 7
    "filterload",   // 8
    "getblocks",    // 9
    "getblocktxn",  // 10
    "getdata",      // 11
    "getheaders",   // 12
    "headers",      // 13
    "inv",          // 14
    "mempool",      // 15
    "merkleblock",  // 16
    "notfound",     // 17
    "ping",         // 18
    "pong",         // 19
    "sendcmpct",    // 20
    "tx",           // 21
    "getcfilters",  // 22
    "cfilter",      // 23
    "getcfheaders", // 24
    "cfheaders",    // 25
    "getcfcheckpt", // 26
    "cfcheckpt",    // 27
    "addrv2"        // 28
};

const std::string& to_command(uint8_t identifier) NOEXCEPT
{
    static const std::string unassigned{};

    if (is_zero(identifier) || identifier > commands.size())
        return unassigned;

    return commands.at(sub1(identifier));
}

uint8_t to_identifier(const std::string& command) NOEXCEPT
{
    const auto it = std::find(commands.begin(), commands.end(), command);

    if (it == commands.end())
        return 0;

    return possible_narrow_sign_cast<uint8_t>(
        add1(std::distance(commands.begin(), it)));
}

} // namespace privacy
} // namespace network
} // namespace libbitcoin
