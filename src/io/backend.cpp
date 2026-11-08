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
#include "backend.hpp"

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace io {
namespace detail {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

void backend::set_result(operation* op, const code& ec, size_t bytes) NOEXCEPT
{
    const auto canceled = op->cancels != op->owner->cancels.load();
    op->ec = ec && canceled ? code{ error::operation_canceled } : ec;
    op->bytes = op->ec ? zero : bytes;
}

handler backend::to_completion(operation* op) NOEXCEPT
{
    return [op]() NOEXCEPT
    {
        const std::unique_ptr<operation> owned{ op };
        auto handler = std::move(op->handler);
        const auto bytes = op->bytes;
        const auto ec = op->ec;
        op->executor.dispatch(
            [handler = std::move(handler), ec, bytes]() NOEXCEPT
            {
                handler(ec, bytes);
            });
    };
}

BC_POP_WARNING()

} // namespace detail
} // namespace io
} // namespace network
} // namespace libbitcoin
