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
#include <bitcoin/network/noise/context.hpp>

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace noise {

using namespace system;

context::context(const ec_secret& static_secret,
    const certificate::data& issued) NOEXCEPT
  : context(static_secret, issued, {}, zero, false)
{
}

context::context(const ec_secret& static_secret, const ec_secret& authority,
    uint32_t validity) NOEXCEPT
  : context(static_secret, {}, authority, validity, true)
{
}

context::context(const ec_secret& static_secret,
    const certificate::data& issued, const ec_secret& authority,
    uint32_t validity, bool issuing) NOEXCEPT
  : static_secret_(static_secret),
    certificate_(issued),
    authority_secret_(authority),
    validity_(validity)
{
    hash_digest auxiliary{};
    maybe_random::fill(auxiliary);
    if (!certificate::to_xonly(server_, static_secret_) ||
        !ellswift::create(static_key_, static_secret_, auxiliary))
        return;

    if (issuing)
    {
        ec_xonly key{};
        if (!certificate::to_xonly(key, authority_secret_))
            return;

        authority_.emplace(key);
    }

    valid_ = true;
}

bool context::valid() const NOEXCEPT
{
    return valid_;
}

const ec_secret& context::static_secret() const NOEXCEPT
{
    return static_secret_;
}

const cipher::key& context::static_key() const NOEXCEPT
{
    return static_key_;
}

const std::optional<ec_xonly>& context::authority() const NOEXCEPT
{
    return authority_;
}

bool context::issue(certificate::data& out, uint32_t now) const NOEXCEPT
{
    if (!authority_.has_value())
    {
        out = certificate_;
        return true;
    }

    return certificate::sign(out, authority_secret_, server_,
        floored_subtract(now, validity_), ceilinged_add(now, validity_));
}

} // namespace noise
} // namespace network
} // namespace libbitcoin
