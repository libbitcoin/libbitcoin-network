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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_TAGS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_TAGS_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/enums/version.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

DECLARE_JSON_TAG_INVOKE(version);
DECLARE_JSON_TAG_INVOKE(value_t);
DECLARE_JSON_TAG_INVOKE(identity_t);
DECLARE_JSON_TAG_INVOKE(request_t);
DECLARE_JSON_TAG_INVOKE(response_t);

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
