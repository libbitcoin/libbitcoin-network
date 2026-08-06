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
#include "../../test.hpp"

using namespace network::rpc;

// setup
// -----------------------------------------------------------------------------

struct mixed_methods
{
    static constexpr std::tuple methods
    {
        method<"alpha">{},
        method<"beta", bool>{ unimplemented, "flag" },
        method<"gamma", bool>{ "flag" },
        method<"delta">{ unimplemented }
    };
};

struct served_methods
{
    static constexpr std::tuple methods
    {
        method<"alpha">{},
        method<"beta">{}
    };
};

struct refused_methods
{
    static constexpr std::tuple methods
    {
        method<"alpha">{ unimplemented },
        method<"beta">{ unimplemented }
    };
};

constexpr std::string_view to_view(const auto& data) NOEXCEPT
{
    return { data.data(), data.size() };
}

constexpr auto mixed = method_names<mixed_methods::methods>();
constexpr auto served = method_names<served_methods::methods>();
constexpr auto refused = method_names<refused_methods::methods>();

// method_names
// -----------------------------------------------------------------------------

// Unimplemented methods are excluded and interface order is retained.
static_assert(to_view(mixed) == "alpha gamma");

// Single space delimiter, no trailing delimiter.
static_assert(to_view(served) == "alpha beta");

// None implemented, no underflow of the delimiter deduction.
static_assert(refused.empty());

// publish
// -----------------------------------------------------------------------------

using mixed_interface = publish<mixed_methods>;
static_assert(mixed_interface::size == 4u);
static_assert(mixed_interface::mode == grouping::either);
static_assert(method_at<mixed_methods::methods, 3>::name == "delta");
static_assert(is_same_type<method_at<mixed_methods::methods, 1>, method<"beta", bool>>);
