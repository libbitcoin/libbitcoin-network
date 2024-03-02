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

// "The constructors of std::shared_ptr detect the presence of an unambiguous
// and accessible (i.e. public inheritance is mandatory)
// enable_shared_from_this base and assign the newly created std::shared_ptr
// to weak-this if not already owned by a live std::shared_ptr.
// Constructing a std::shared_ptr for an object that is already managed by
// another std::shared_ptr will not consult weak-this and is undefined behavior.
// It is permitted to call shared_from_this only on a previously shared object,
// i.e. on an object managed by std::shared_ptr<T>. Otherwise,
// std::bad_weak_ptr is thrown(by the shared_ptr constructor).."
// en.cppreference.com/w/cpp/memory/enable_shared_from_this

/// Base instance must be downcastable to Derived.
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

/// Sibling instance must be castable to Derived:Base.
/// Because enable_shared_from_this/base do not support multiple inheritance.
/// Avoids diamond inheritance ambiguity by requiring a primary inheritance
/// linear path (sibling) from which classes in an independent path (Base)
/// may obtain a shared pointer within their own path (Derived).
template <class Base, class Sibling>
class enable_shared_from_sibling
{
public:
    DELETE_COPY_MOVE(enable_shared_from_sibling);

    enable_shared_from_sibling() NOEXCEPT;

    /// Must be a polymorphic type (to use dynamic_cast).
    virtual ~enable_shared_from_sibling() NOEXCEPT;

    /// Simplifies capture of the shared pointer for a nop handler.
    void nop() volatile NOEXCEPT;

protected:
    /// Sibling (not Derived:Base) implements enable_shared_from...
    /// Use in Derived to create shared instance of Derived from its Sibling.
    /// Undefined behavior if cast from Sibling* to Derived* not well formed.
    template <class Derived, bc::if_base_of<Base, Derived> = true>
    std::shared_ptr<Derived> shared_from_sibling() NOEXCEPT;
};

// class foo__ : enabled_shared_from_base<foo__>
// class foo_ : foo__
// class foo: foo_
// auto fooptr = foo.shared_from_this()
// auto foo_ptr = foo.shared_from_base<foo_>()
// auto foo__ptr = foo.shared_from_base<foo__>()
//
// class bar__ : enabled_shared_from_sibling<bar__, foo>
// class bar_ : bar__
// class bar : bar_
// bar__/bar_/bar must be joined with foo.
//
// class foobar : public foo, public bar
// auto barptr = foobar.shared_from_sibling<bar>()
// auto bar_ptr = foobar.shared_from_sibling<bar_>()
// auto bar__ptr = foobar.shared_from_sibling<bar__>()
// auto fooptr = foobar.shared_from_base<foo>()
// auto foo_ptr = foobar.shared_from_base<foo_>()
// auto foo__ptr = foobar.shared_from_base<foo__>()

} // namespace network
} // namespace libbitcoin

#include <bitcoin/network/impl/async/enable_shared_from_base.ipp>

#endif
