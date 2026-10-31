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
#ifndef LIBBITCOIN_NETWORK_IO_STRAND_HPP
#define LIBBITCOIN_NETWORK_IO_STRAND_HPP

#include <memory>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/io/context.hpp>

namespace libbitcoin {
namespace network {
namespace io {

/// Thread safe. An executor whose handlers run in post order, one at a time.
/// Copies are the same strand.
class BCT_API strand
  : public executor
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(strand);

    /// A new strand of the context of the executor.
    explicit strand(const executor& executor) NOEXCEPT;
    explicit strand(io::context& context) NOEXCEPT;
};

} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
