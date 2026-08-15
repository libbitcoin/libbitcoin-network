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

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/interfaces/peer_registry.hpp>

namespace libbitcoin {
namespace network {
namespace privacy {

std::string to_command(uint8_t identifier) NOEXCEPT
{
    using registry = rpc::peer_registry;
    const auto index = registry::index(identifier);
    return index == registry::unknown ? std::string{} :
        std::string{ registry::commands().at(index) };
}

uint8_t to_identifier(const std::string& command) NOEXCEPT
{
    using registry = rpc::peer_registry;
    const auto index = registry::index(command);
    return index == registry::unknown ? 0_u8 :
        registry::identifiers().at(index);
}

} // namespace privacy
} // namespace network
} // namespace libbitcoin
