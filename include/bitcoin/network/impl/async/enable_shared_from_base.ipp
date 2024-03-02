/**
 * Copyright (c) 2011-2023 libbitcoin developers (see AUTHORS)
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
#ifndef LIBBITCOIN_NETWORK_ASYNC_ENABLE_SHARED_FROM_BASE_IPP
#define LIBBITCOIN_NETWORK_ASYNC_ENABLE_SHARED_FROM_BASE_IPP

#include <memory>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {
    
// enable_shared_from_base : enable_shared_from_this
// ----------------------------------------------------------------------------

template <class Base>
void enable_shared_from_base<Base>::
nop() volatile NOEXCEPT
{
}

template <class Base>
template <class Derived, bc::if_base_of<Base, Derived>>
std::shared_ptr<Derived> enable_shared_from_base<Base>::
shared_from_base() NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    return std::static_pointer_cast<Derived>(this->shared_from_this());
    BC_POP_WARNING()
}

// enable_shared_from_sibling
// ----------------------------------------------------------------------------

template <class Base, class Sibling>
enable_shared_from_sibling<Base, Sibling>::
enable_shared_from_sibling() NOEXCEPT
{
}

template <class Base, class Sibling>
enable_shared_from_sibling<Base, Sibling>::
~enable_shared_from_sibling() NOEXCEPT
{
}

template <class Base, class Sibling>
void enable_shared_from_sibling<Base, Sibling>::
nop() volatile NOEXCEPT
{
}

template <class Base, class Sibling>
template <class Derived, bc::if_base_of<Base, Derived>>
std::shared_ptr<Derived> enable_shared_from_sibling<Base, Sibling>::
shared_from_sibling() NOEXCEPT
{
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    // Obtain shared pointer to object from Sibling class inheritance path.
    const auto sibling = dynamic_cast<Sibling*>(this)->shared_from_this();
    BC_POP_WARNING()

    // Cast pointer back to Derived (from Base) class it was created from.
    return std::dynamic_pointer_cast<Derived>(sibling);
}

} // namespace network
} // namespace libbitcoin

#endif
