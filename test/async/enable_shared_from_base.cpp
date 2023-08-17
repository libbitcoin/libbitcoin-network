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
#include "../test.hpp"

BOOST_AUTO_TEST_SUITE(enable_shared_from_base_tests)

class base_class
  : public enable_shared_from_base<base_class>
{
public:
    typedef std::shared_ptr<base_class> ptr;

    template <typename Derived>
    typename Derived::ptr self() NOEXCEPT
    {
        return shared_from_base<Derived>();
    }

    bool base_method(bool value) const NOEXCEPT
    {
        return value;
    }
};

class derived_class
  : public base_class
{
public:
    typedef std::shared_ptr<derived_class> ptr;

    bool derived_method(bool value) const NOEXCEPT
    {
        return value;
    }
};

BOOST_AUTO_TEST_CASE(enable_shared_from_base__nop__nop)
{
    std::make_shared<base_class>()->shared_from_this()->nop();
    BOOST_REQUIRE(true);
}

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_this__from_base__base)
{
    const auto alpha = std::make_shared<base_class>();
    const auto beta = alpha->shared_from_this();
    BOOST_REQUIRE(beta->base_method(true));
}

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_base__from_base__base)
{
    const auto alpha = std::make_shared<base_class>();
    const auto beta = alpha->self<base_class>();
    BOOST_REQUIRE(beta->base_method(true));
}

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_this__from_derived__base)
{
    const auto alpha = std::make_shared<derived_class>();
    const auto beta = alpha->shared_from_this();
    BOOST_REQUIRE(beta->base_method(true));
}

BOOST_AUTO_TEST_CASE(enable_shared_from_base__shared_from_base__from_derived__derived)
{
    const auto alpha = std::make_shared<derived_class>();
    const auto beta = alpha->self<derived_class>();
    BOOST_REQUIRE(beta->derived_method(true));
}

BOOST_AUTO_TEST_SUITE_END()
