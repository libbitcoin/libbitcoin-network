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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PAYLOAD_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PAYLOAD_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace json {

/// Content passed to/from reader/writer via request/response.
/// `static uint64_t size(const payload&)` must be defined for beast to produce
/// `content_length`, otherwise the response is chunked. Predetermining size
/// would have the effect of eliminating the benefit of streaming serialize.
struct payload
{
    /// JSON DOM.
    boost::json::value model{};

    /// Writer serialization buffer (max size, allocated on write).
    mutable http::flat_buffer_ptr buffer{};
};


} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
