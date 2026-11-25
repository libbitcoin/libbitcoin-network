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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_SV2_ENUMS_IDENTIFIERS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_SV2_ENUMS_IDENTIFIERS_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {
namespace identifiers {

enum identifier_t : uint8_t
{
    setup_connection = 0x00,
    setup_connection_success = 0x01,
    setup_connection_error = 0x02,
    channel_endpoint_changed = 0x03,
    reconnect = 0x04,
    open_standard_mining_channel = 0x10,
    open_standard_mining_channel_success = 0x11,
    open_mining_channel_error = 0x12,
    open_extended_mining_channel = 0x13,
    open_extended_mining_channel_success = 0x14,
    new_mining_job = 0x15,
    update_channel = 0x16,
    update_channel_error = 0x17,
    close_channel = 0x18,
    set_extranonce_prefix = 0x19,
    submit_shares_standard = 0x1a,
    submit_shares_extended = 0x1b,
    submit_shares_success = 0x1c,
    submit_shares_error = 0x1d,
    new_extended_mining_job = 0x1f,
    set_new_prev_hash = 0x20,
    set_target = 0x21,
    set_custom_mining_job = 0x22,
    set_custom_mining_job_success = 0x23,
    set_custom_mining_job_error = 0x24,
    set_group_channel = 0x25
};

} // namespace identifiers
} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
