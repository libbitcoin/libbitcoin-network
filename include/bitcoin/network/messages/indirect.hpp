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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_INDIRECT_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_INDIRECT_HPP

#include <memory>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {

/// A value held in allocated storage, with value semantics (copies copy the
/// value). A moved-from instance holds no value and may only be assigned or
/// destroyed.
template <typename Type>
class indirect
{
public:
    explicit indirect(Type&& value) NOEXCEPT
      : value_(std::make_unique<Type>(std::move(value)))
    {
    }

    explicit indirect(const Type& value) NOEXCEPT
      : value_(std::make_unique<Type>(value))
    {
    }

    indirect(const indirect& other) NOEXCEPT
      : value_(std::make_unique<Type>(*other))
    {
    }

    indirect(indirect&& other) NOEXCEPT = default;

    indirect& operator=(const indirect& other) NOEXCEPT
    {
        if (this != &other)
            value_ = std::make_unique<Type>(*other);

        return *this;
    }

    indirect& operator=(indirect&& other) NOEXCEPT = default;
    ~indirect() NOEXCEPT = default;

    Type& operator*() NOEXCEPT
    {
        BC_ASSERT(value_);
        return *value_;
    }

    const Type& operator*() const NOEXCEPT
    {
        BC_ASSERT(value_);
        return *value_;
    }

    Type* operator->() NOEXCEPT
    {
        return &**this;
    }

    const Type* operator->() const NOEXCEPT
    {
        return &**this;
    }

private:
    std::unique_ptr<Type> value_;
};

} // namespace network
} // namespace libbitcoin

#endif
