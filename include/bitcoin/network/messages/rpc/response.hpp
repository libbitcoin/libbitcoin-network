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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_RESPONSE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_RESPONSE_HPP

#include <memory>
#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/heading.hpp>
#include <bitcoin/network/messages/rpc/enums/identifier.hpp>
#include <bitcoin/network/messages/rpc/enums/status.hpp>
#include <bitcoin/network/messages/rpc/enums/version.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

struct BCT_API response
{
    typedef std::shared_ptr<const response> cptr;

    static const identifier id;
    static const std::string command;

    /// Canonical serialization buffer size (includes 1 OWS SP).
    size_t size() const NOEXCEPT;

    static cptr deserialize(const system::data_chunk& data) NOEXCEPT;
    static response deserialize(system::reader& source) NOEXCEPT;

    bool serialize(const system::data_slab& data) const NOEXCEPT;
    void serialize(system::writer& sink) const NOEXCEPT;

    rpc::version version;
    rpc::status status;
    heading::fields fields{};
};

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
