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

BC_AUTO_TEST_SUITE(any_tests)

using namespace rpc;

BC_AUTO_TEST_CASE(any__default_constructor__always__empty)
{
    const any instance{};
    BC_REQUIRE(!instance.has_value());
    BC_REQUIRE(!static_cast<bool>(instance));
    BC_REQUIRE(!instance.get<int>());
    BC_REQUIRE(!instance.holds_alternative<int>());
}

BC_AUTO_TEST_CASE(any__shared_ptr_constructor__valid_pointer__expected_state)
{
    const auto ptr = std::make_shared<int>(42);
    const any instance(ptr);
    BC_REQUIRE(instance.has_value());
    BC_REQUIRE(static_cast<bool>(instance));
    BC_REQUIRE(instance.holds_alternative<int>());
    BC_REQUIRE_EQUAL(*instance.get<int>(), 42);
    BC_REQUIRE(!instance.holds_alternative<double>());
}

BC_AUTO_TEST_CASE(any__shared_ptr_constructor__null_pointer__has_value_false)
{
    const any instance(std::shared_ptr<int>{});
    BC_REQUIRE(!instance.has_value());
    BC_REQUIRE(!instance.holds_alternative<int>());
}

BC_AUTO_TEST_CASE(any__emplace__integral__expected_value)
{
    any instance{};
    instance.emplace<int>(123);
    BC_REQUIRE_EQUAL(*instance.get<int>(), 123);
}

BC_AUTO_TEST_CASE(any__emplace__struct__expected_value)
{
    struct foo { int value{}; };
    any instance{};
    instance.emplace<foo>(456);
    BC_REQUIRE_EQUAL(instance.get<foo>()->value, 456);
    BC_REQUIRE(!instance.get<int>());
}

BC_AUTO_TEST_CASE(any__get__wrong_type__nullptr)
{
    const any instance{ std::make_shared<int>(42) };
    BC_REQUIRE(!instance.get<double>());
}

BC_AUTO_TEST_CASE(any__get__empty__nullptr)
{
    BC_REQUIRE(!any{}.get<int>());
}

BC_AUTO_TEST_CASE(any__as__correct_type__returns_shared_ptr)
{
    const any instance{ std::make_shared<int>(42) };
    BC_REQUIRE_NO_THROW(instance.as<int>());
    BC_REQUIRE_EQUAL(*instance.as<int>(), 42);
}

BC_AUTO_TEST_CASE(any__as__wrong_type__throws_bad_any_cast)
{
    const any instance{ std::make_shared<int>(42) };
    BC_REQUIRE_THROW(instance.as<double>(), std::bad_any_cast);
}

BC_AUTO_TEST_CASE(any__as__empty__throws_bad_any_cast)
{
    const any instance{};
    BC_REQUIRE_THROW(instance.as<int>(), std::bad_any_cast);
}

BC_AUTO_TEST_CASE(any__holds_alternative__correct_type__true)
{
    const any instance{ std::make_shared<int>(0) };
    BC_REQUIRE(instance.holds_alternative<int>());
}

BC_AUTO_TEST_CASE(any__holds_alternative__wrong_type__false)
{
    const any instance{ std::make_shared<int>(0) };
    BC_REQUIRE(!instance.holds_alternative<float>());
}

BC_AUTO_TEST_CASE(any__holds_alternative__cv_qualifiers__exact_match_only)
{
    const any a{ std::make_shared<int>(0) };
    BC_REQUIRE(a.holds_alternative<int>());
    BC_REQUIRE(!a.holds_alternative<const int>());

    const any b{ std::make_shared<const int>(0) };
    BC_REQUIRE(b.holds_alternative<const int>());
    BC_REQUIRE(!b.holds_alternative<int>());
}

BC_AUTO_TEST_CASE(any__reset__populated__becomes_empty)
{
    any instance{ std::make_shared<int>(42) };
    instance.reset();
    BC_REQUIRE(!instance.has_value());
    BC_REQUIRE(!static_cast<bool>(instance));
    BC_REQUIRE(!instance.get<int>());
}

BC_AUTO_TEST_CASE(any__copy_constructor__shares_ownership)
{
    any foo{};
    foo.emplace<int>(42);
    BC_REQUIRE(foo.has_value());

    const any bar{ foo };
    BC_REQUIRE(foo.has_value());
    BC_REQUIRE(bar.has_value());

    // foo still has a non-nullptr value.
    const auto pfoo = foo.get<int>();
    BC_REQUIRE(pfoo);
    BC_REQUIRE_EQUAL(*pfoo, 42);

    const auto pbar = bar.get<int>();
    BC_REQUIRE(pbar);
    BC_REQUIRE_EQUAL(*pbar, 42);
}

BC_AUTO_TEST_CASE(any__copy_assignment__shares_ownership)
{
    any foo{};
    foo.emplace<int>(42);
    BC_REQUIRE(foo.has_value());

    const auto bar = foo;
    BC_REQUIRE(foo.has_value());
    BC_REQUIRE(bar.has_value());

    // foo still has a non-nullptr value.
    const auto pfoo = foo.get<int>();
    BC_REQUIRE(pfoo);
    BC_REQUIRE_EQUAL(*pfoo, 42);

    const auto pbar = bar.get<int>();
    BC_REQUIRE(pbar);
    BC_REQUIRE_EQUAL(*pbar, 42);
}

BC_AUTO_TEST_CASE(any__move_constructor__transfers_ownership)
{
    any foo{};
    foo.emplace<int>(42);
    BC_REQUIRE(foo.has_value());

    // foo has been explicitly cleared, has no inner value.
    const any bar(std::move(foo));
    BC_REQUIRE(!foo.has_value());
    BC_REQUIRE(bar.has_value());

    const auto pbar = bar.get<int>();
    BC_REQUIRE(pbar);
    BC_REQUIRE_EQUAL(*pbar, 42);
}

BC_AUTO_TEST_CASE(any__move_assignment__transfers_ownership)
{
    any foo{};
    foo.emplace<int>(42);
    BC_REQUIRE(foo.has_value());

    // foo has been explicitly cleared, has no inner value.
    const auto bar = std::move(std::move(foo));
    BC_REQUIRE(!foo.has_value());
    BC_REQUIRE(bar.has_value());

    const auto pbar = bar.get<int>();
    BC_REQUIRE(pbar);
    BC_REQUIRE_EQUAL(*pbar, 42);
}

BC_AUTO_TEST_CASE(any__const_access__expected)
{
    const any instance{ std::make_shared<int>(123) };
    BC_REQUIRE(instance.has_value());
    BC_REQUIRE(instance.holds_alternative<int>());
    BC_REQUIRE_EQUAL(*instance.get<int>(), 123);
    BC_REQUIRE_NO_THROW(instance.as<int>());
}

BC_AUTO_TEST_SUITE_END()