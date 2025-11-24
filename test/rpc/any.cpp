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

BOOST_AUTO_TEST_SUITE(any_tests)

using namespace rpc;

BOOST_AUTO_TEST_CASE(any__default_constructor__always__empty)
{
    const any instance{};
    BOOST_REQUIRE(!instance.has_value());
    BOOST_REQUIRE(!static_cast<bool>(instance));
    BOOST_REQUIRE(!instance.get<int>());
    BOOST_REQUIRE(!instance.holds_alternative<int>());
}

BOOST_AUTO_TEST_CASE(any__shared_ptr_constructor__valid_pointer__expected_state)
{
    const auto ptr = std::make_shared<int>(42);
    const any instance(ptr);
    BOOST_REQUIRE(instance.has_value());
    BOOST_REQUIRE(static_cast<bool>(instance));
    BOOST_REQUIRE(instance.holds_alternative<int>());
    BOOST_REQUIRE_EQUAL(*instance.get<int>(), 42);
    BOOST_REQUIRE(!instance.holds_alternative<double>());
}

BOOST_AUTO_TEST_CASE(any__shared_ptr_constructor__null_pointer__has_value_false)
{
    const any instance(std::shared_ptr<int>{});
    BOOST_REQUIRE(!instance.has_value());
    BOOST_REQUIRE(!instance.holds_alternative<int>());
}

BOOST_AUTO_TEST_CASE(any__emplace__integral__expected_value)
{
    any instance{};
    instance.emplace<int>(123);
    BOOST_REQUIRE_EQUAL(*instance.get<int>(), 123);
}

BOOST_AUTO_TEST_CASE(any__emplace__struct__expected_value)
{
    struct foo { int value{}; };
    any instance{};
    instance.emplace<foo>(456);
    BOOST_REQUIRE_EQUAL(instance.get<foo>()->value, 456);
    BOOST_REQUIRE(!instance.get<int>());
}

BOOST_AUTO_TEST_CASE(any__get__wrong_type__nullptr)
{
    const any instance{ std::make_shared<int>(42) };
    BOOST_REQUIRE(!instance.get<double>());
}

BOOST_AUTO_TEST_CASE(any__get__empty__nullptr)
{
    BOOST_REQUIRE(!any{}.get<int>());
}

BOOST_AUTO_TEST_CASE(any__as__correct_type__returns_shared_ptr)
{
    const any instance{ std::make_shared<int>(42) };
    BOOST_REQUIRE_NO_THROW(instance.as<int>());
    BOOST_REQUIRE_EQUAL(*instance.as<int>(), 42);
}

BOOST_AUTO_TEST_CASE(any__as__wrong_type__throws_bad_any_cast)
{
    const any instance{ std::make_shared<int>(42) };
    BOOST_REQUIRE_THROW(instance.as<double>(), std::bad_any_cast);
}

BOOST_AUTO_TEST_CASE(any__as__empty__throws_bad_any_cast)
{
    const any instance{};
    BOOST_REQUIRE_THROW(instance.as<int>(), std::bad_any_cast);
}

BOOST_AUTO_TEST_CASE(any__holds_alternative__correct_type__true)
{
    const any instance{ std::make_shared<int>(0) };
    BOOST_REQUIRE(instance.holds_alternative<int>());
}

BOOST_AUTO_TEST_CASE(any__holds_alternative__wrong_type__false)
{
    const any instance{ std::make_shared<int>(0) };
    BOOST_REQUIRE(!instance.holds_alternative<float>());
}

BOOST_AUTO_TEST_CASE(any__holds_alternative__cv_qualifiers__exact_match_only)
{
    const any a{ std::make_shared<int>(0) };
    BOOST_REQUIRE(a.holds_alternative<int>());
    BOOST_REQUIRE(!a.holds_alternative<const int>());

    const any b{ std::make_shared<const int>(0) };
    BOOST_REQUIRE(b.holds_alternative<const int>());
    BOOST_REQUIRE(!b.holds_alternative<int>());
}

BOOST_AUTO_TEST_CASE(any__reset__populated__becomes_empty)
{
    any instance{ std::make_shared<int>(42) };
    instance.reset();
    BOOST_REQUIRE(!instance.has_value());
    BOOST_REQUIRE(!static_cast<bool>(instance));
    BOOST_REQUIRE(!instance.get<int>());
}

BOOST_AUTO_TEST_CASE(any__copy_constructor__shares_ownership)
{
    any foo{};
    foo.emplace<int>(42);
    BOOST_REQUIRE(foo.has_value());

    const any bar{ foo };
    BOOST_REQUIRE(foo.has_value());
    BOOST_REQUIRE(bar.has_value());

    // foo still has a non-nullptr value.
    const auto pfoo = foo.get<int>();
    BOOST_REQUIRE(pfoo);
    BOOST_REQUIRE_EQUAL(*pfoo, 42);

    const auto pbar = bar.get<int>();
    BOOST_REQUIRE(pbar);
    BOOST_REQUIRE_EQUAL(*pbar, 42);
}

BOOST_AUTO_TEST_CASE(any__copy_assignment__shares_ownership)
{
    any foo{};
    foo.emplace<int>(42);
    BOOST_REQUIRE(foo.has_value());

    const auto bar = foo;
    BOOST_REQUIRE(foo.has_value());
    BOOST_REQUIRE(bar.has_value());

    // foo still has a non-nullptr value.
    const auto pfoo = foo.get<int>();
    BOOST_REQUIRE(pfoo);
    BOOST_REQUIRE_EQUAL(*pfoo, 42);

    const auto pbar = bar.get<int>();
    BOOST_REQUIRE(pbar);
    BOOST_REQUIRE_EQUAL(*pbar, 42);
}

BOOST_AUTO_TEST_CASE(any__move_constructor__transfers_ownership)
{
    any foo{};
    foo.emplace<int>(42);
    BOOST_REQUIRE(foo.has_value());

    // foo has been explicitly cleared, has no inner value.
    const any bar(std::move(foo));
    BOOST_REQUIRE(!foo.has_value());
    BOOST_REQUIRE(bar.has_value());

    const auto pbar = bar.get<int>();
    BOOST_REQUIRE(pbar);
    BOOST_REQUIRE_EQUAL(*pbar, 42);
}

BOOST_AUTO_TEST_CASE(any__move_assignment__transfers_ownership)
{
    any foo{};
    foo.emplace<int>(42);
    BOOST_REQUIRE(foo.has_value());

    // foo has been explicitly cleared, has no inner value.
    const auto bar = std::move(std::move(foo));
    BOOST_REQUIRE(!foo.has_value());
    BOOST_REQUIRE(bar.has_value());

    const auto pbar = bar.get<int>();
    BOOST_REQUIRE(pbar);
    BOOST_REQUIRE_EQUAL(*pbar, 42);
}

BOOST_AUTO_TEST_CASE(any__const_access__expected)
{
    const any instance{ std::make_shared<int>(123) };
    BOOST_REQUIRE(instance.has_value());
    BOOST_REQUIRE(instance.holds_alternative<int>());
    BOOST_REQUIRE_EQUAL(*instance.get<int>(), 123);
    BOOST_REQUIRE_NO_THROW(instance.as<int>());
}

BOOST_AUTO_TEST_SUITE_END()