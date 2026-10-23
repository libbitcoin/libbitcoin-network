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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_STREAMABLE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_STREAMABLE_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// A value that writes its own encoding (carried as an rpc any_t).
class streamable
{
public:
    typedef std::shared_ptr<const streamable> cptr;

    virtual ~streamable() = default;

    /// The size of the encoding.
    virtual size_t size() const NOEXCEPT = 0;

    /// Write the encoding to the sink.
    virtual void write(system::write::bytes::fast& sink) const NOEXCEPT = 0;
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
