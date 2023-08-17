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
#ifndef LIBBITCOIN_NETWORK_ASYNC_ENABLE_SHARED_FROM_BASE_HPP
#define LIBBITCOIN_NETWORK_ASYNC_ENABLE_SHARED_FROM_BASE_HPP

#include <memory>
#include <bitcoin/system.hpp>

namespace libbitcoin {
namespace network {

/// Thread safe, base class.
/// Empty base optimization using CRTP.
/// Because enable_shared_from_this does not support inheritance.
template <class Base>
class enable_shared_from_base
  : public std::enable_shared_from_this<Base>
{
public:
    /// Simplifies capture of the shared pointer for a nop handler.
    void nop() volatile NOEXCEPT;

protected:
    /// Use in derived class to create shared instance of self.
    template <class Derived, bc::if_base_of<Base, Derived> = true>
    std::shared_ptr<Derived> shared_from_base() NOEXCEPT;
};

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/enable_shared_from_base.ipp>

#endif
