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
#include <bitcoin/network/broadcast/diagnostics.hpp>

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

diagnostics::sink::sink() NOEXCEPT
{
}

void diagnostics::sink::add(row&& value) NOEXCEPT
{
    std::lock_guard lock{ mutex_ };

    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    rows_.push_back(std::move(value));
    BC_POP_WARNING()
}

const diagnostics::rows& diagnostics::sink::captured() const NOEXCEPT
{
    return rows_;
}

diagnostics::diagnostics(const race::ptr& complete, const sink::ptr& captured,
    target group, uint64_t channel) NOEXCEPT
  : race_(complete), sink_(captured), group_(group), channel_(channel)
{
}

bool diagnostics::targets(uint64_t identifier) const NOEXCEPT
{
    return group_ == target::channel && channel_ == identifier;
}

bool diagnostics::targets(target group) const NOEXCEPT
{
    return group_ == target::all || group_ == group;
}

void diagnostics::add(row&& value) const NOEXCEPT
{
    sink_->add(std::move(value));
}

} // namespace network
} // namespace libbitcoin
