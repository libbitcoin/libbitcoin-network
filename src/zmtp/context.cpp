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
#include <bitcoin/network/zmtp/context.hpp>

#include <algorithm>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/zmtp/cipher.hpp>

namespace libbitcoin {
namespace network {
namespace zmtp {

context::context() NOEXCEPT
{
}

context::context(const system::data_chunk& secret) NOEXCEPT
{
    if (secret.size() != cipher::key_size)
        return;

    std::copy(secret.begin(), secret.end(), secret_.begin());
    curve_ = cipher::to_public(public_, secret_);
}

bool context::curve() const NOEXCEPT
{
    return curve_;
}

const cipher::key& context::secret() const NOEXCEPT
{
    return secret_;
}

const cipher::key& context::public_key() const NOEXCEPT
{
    return public_;
}

} // namespace zmtp
} // namespace network
} // namespace libbitcoin
