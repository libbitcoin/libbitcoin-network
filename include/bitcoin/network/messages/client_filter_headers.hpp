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

#ifndef LIBBITCOIN_NETWORK_MESSAGES_CLIENT_FILTER_HEADERS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_CLIENT_FILTER_HEADERS_HPP

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/enums/identifier.hpp>

namespace libbitcoin {
namespace network {
namespace messages {

struct BCT_API client_filter_headers
{
    typedef std::shared_ptr<const client_filter_headers> ptr;

    static const identifier id;
    static const std::string command;
    static const uint32_t version_minimum;
    static const uint32_t version_maximum;

    static client_filter_headers deserialize(uint32_t version,
        system::reader& source) noexcept;
    void serialize(uint32_t version, system::writer& sink) const noexcept;
    size_t size(uint32_t version) const noexcept;

    uint8_t filter_type;
    system::hash_digest stop_hash;
    system::hash_digest previous_filter_header;
    system::hash_list filter_hashes;
};

} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
