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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_SV2_SV2_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_SV2_SV2_HPP

#include <bitcoin/network/messages/sv2/detail/channel_endpoint_changed.hpp>
#include <bitcoin/network/messages/sv2/detail/close_channel.hpp>
#include <bitcoin/network/messages/sv2/detail/new_extended_mining_job.hpp>
#include <bitcoin/network/messages/sv2/detail/new_mining_job.hpp>
#include <bitcoin/network/messages/sv2/detail/open_extended_mining_channel.hpp>
#include <bitcoin/network/messages/sv2/detail/open_extended_mining_channel_success.hpp>
#include <bitcoin/network/messages/sv2/detail/open_mining_channel_error.hpp>
#include <bitcoin/network/messages/sv2/detail/open_standard_mining_channel.hpp>
#include <bitcoin/network/messages/sv2/detail/open_standard_mining_channel_success.hpp>
#include <bitcoin/network/messages/sv2/detail/reconnect.hpp>
#include <bitcoin/network/messages/sv2/detail/set_custom_mining_job.hpp>
#include <bitcoin/network/messages/sv2/detail/set_custom_mining_job_error.hpp>
#include <bitcoin/network/messages/sv2/detail/set_custom_mining_job_success.hpp>
#include <bitcoin/network/messages/sv2/detail/set_extranonce_prefix.hpp>
#include <bitcoin/network/messages/sv2/detail/set_group_channel.hpp>
#include <bitcoin/network/messages/sv2/detail/set_new_prev_hash.hpp>
#include <bitcoin/network/messages/sv2/detail/set_target.hpp>
#include <bitcoin/network/messages/sv2/detail/setup_connection.hpp>
#include <bitcoin/network/messages/sv2/detail/setup_connection_error.hpp>
#include <bitcoin/network/messages/sv2/detail/setup_connection_success.hpp>
#include <bitcoin/network/messages/sv2/detail/submit_shares_error.hpp>
#include <bitcoin/network/messages/sv2/detail/submit_shares_extended.hpp>
#include <bitcoin/network/messages/sv2/detail/submit_shares_standard.hpp>
#include <bitcoin/network/messages/sv2/detail/submit_shares_success.hpp>
#include <bitcoin/network/messages/sv2/detail/update_channel.hpp>
#include <bitcoin/network/messages/sv2/detail/update_channel_error.hpp>
#include <bitcoin/network/messages/sv2/enums/identifiers.hpp>
#include <bitcoin/network/messages/sv2/body.hpp>
#include <bitcoin/network/messages/sv2/heading.hpp>
#include <bitcoin/network/messages/sv2/message.hpp>
#include <bitcoin/network/messages/sv2/types.hpp>

#endif
