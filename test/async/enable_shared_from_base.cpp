/**
 * Copyright (c) 2011-2025 libbitcoin developers (see AUTHORS)
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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(enable_shared_from_base_tests)

class base_class
  : public enable_shared_from_base<base_class>
{
public:
    typedef std::shared_ptr<base_class> ptr;
    virtual ~base_class() {};

    template <class Derived>
    typename Derived::ptr self() NOEXCEPT
    {
        return shared_from_base<Derived>();
    }
    
    template <class Sibling, class Shared>
    typename Sibling::ptr sibling() NOEXCEPT
    {
        return shared_from_sibling<Sibling, Shared>();
    }

    virtual bool base_method() const NOEXCEPT
    {
        return false;
    }
};

class derived_left
  : public base_class
{
public:
    typedef std::shared_ptr<derived_left> ptr;
    virtual ~derived_left() {};

    bool base_method() const NOEXCEPT override
    {
        return true;
    }

    bool left_method() const NOEXCEPT
    {
        return true;
    }
};

class underived_right
{
public:
    typedef std::shared_ptr<underived_right> ptr;
    virtual ~underived_right() {};

    bool right_method() const NOEXCEPT
    {
        return true;
    }
};

class multiple : public derived_left, public underived_right
{
public:
    typedef std::shared_ptr<multiple> ptr;
    virtual ~multiple() {};

    bool multiple_method() const NOEXCEPT
    {
        return true;
    }
};

// enable_shared_from_base

BOOST_AUTO_TEST_CASE(enable_shared_from_base__nop__nop)
{
    std::make_shared<base_class>()->shared_from_this()->nop();
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_this__from_base__base)
{
    const auto base = std::make_shared<base_class>();
    const auto self = base->shared_from_this();
    BOOST_REQUIRE(!base->base_method());
    BOOST_REQUIRE(!self->base_method());
}

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_base__from_base__base)
{
    const auto base = std::make_shared<base_class>();
    const auto self = base->self<base_class>();
    BOOST_REQUIRE(!base->base_method());
    BOOST_REQUIRE(!self->base_method());
}

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_this__from_derived__polymorphic)
{
    const auto left = std::make_shared<derived_left>();
    const auto base = left->shared_from_this();

    // Picks up the left override.
    BOOST_REQUIRE(left->base_method());
    BOOST_REQUIRE(base->base_method());

    // But left is not directly accessible (why we need shared_from_base<>).
    ////BOOST_REQUIRE(left->left_method());
}

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_base__from_base__derived_and_polymorphic)
{
    const auto base = std::static_pointer_cast<base_class>(std::make_shared<derived_left>());
    const auto left = base->self<derived_left>();

    // This is overridden, even in the upcast instance.
    BOOST_REQUIRE(base->base_method());

    // Picks up the left override.
    BOOST_REQUIRE(left->base_method());

    // Derived left is now directly accessible.
    BOOST_REQUIRE(left->left_method());
}

// enable_shared_from_sibling

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_sibling__multiple_derived__expected)
{
    const auto base = std::make_shared<multiple>();
    const auto left = base->sibling<derived_left, base_class>();

    // Works like shared_from_base (but less guarded/performant).
    BOOST_REQUIRE(left->base_method());
    BOOST_REQUIRE(left->left_method());
}

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_sibling__multiple_sibling__expected)
{
    const auto base = std::make_shared<multiple>();
    const auto right = base->sibling<underived_right, base_class>();

    // right is now directly accessible from left, yet right does not implement shared_from_base.
    BOOST_REQUIRE(right->right_method());
}

BOOST_AUTO_TEST_SUITE_END()
