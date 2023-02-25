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
#ifndef LIBBITCOIN_NETWORK_LOG_EVENT_HPP
#define LIBBITCOIN_NETWORK_LOG_EVENT_HPP

#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
namespace event_t {

// Use event_t namespace to prevent pollution of network namesapce.
// Could use class enum, but we want simple conversion to uint8_t.
enum events : uint8_t
{
    stop,
    outbound1,
    outbound2,
    outbound3
};

} // namespace event_t
} // namespace network
} // namespace libbitcoin

#endif
