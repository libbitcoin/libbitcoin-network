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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_ANY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_ANY_HPP

#include <any>
#include <memory>
#include <utility>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

/// Similar to std::any, but preserves fixed stack size through shared_ptr.
/// This allows it to participate in a std::variant type without inflation.
/// Also differs in that a move will fully remove the original inner object.
class any
{
public:
    template <typename Type>
    inline any(const std::shared_ptr<Type>& ptr) NOEXCEPT
      : any{ from_ptr(ptr) }
    {
    }

    template <typename Type>
    inline any(std::shared_ptr<Type>&& ptr) NOEXCEPT
      : any{ from_ptr(std::forward<std::shared_ptr<Type>>(ptr)) }
    {
    }

    DEFAULT_COPY(any);
    ~any() NOEXCEPT = default;

    any() NOEXCEPT
      : inner_{}
    {
    }

    any(any&& other) NOEXCEPT
      : inner_{ std::move(other.inner_) }
    {
        other.inner_.reset();
    }

    any& operator=(any&& other) NOEXCEPT
    {
        if (this != &other)
            inner_ = move(other.inner_);

        other.inner_.reset();
        return *this;
    }

    template <typename Type, typename ...Args>
    inline void emplace(Args&&... args) NOEXCEPT
    {
        const auto ptr = std::make_shared<Type>(std::forward<Args>(args)...);
        inner_ = ptr;

        // Prevent has_value with null contained ponter.
        if (!ptr) inner_.reset();
    }

    template <typename Type>
    inline std::shared_ptr<Type> get() const NOEXCEPT
    {
        return holds_alternative<Type>() ? *get_inner<Type>() :
            std::shared_ptr<Type>{};
    }

    template <typename Type>
    inline const std::shared_ptr<Type>& as() const THROWS
    {
        return holds_alternative<Type>() ? *get_inner<Type>() :
            throw std::bad_any_cast{};
    }

    template <typename Type>
    inline bool holds_alternative() const NOEXCEPT
    {
        return !is_null(get_inner<Type>());
    }

    inline bool has_value() const NOEXCEPT
    {
        return inner_.has_value();
    }

    inline operator bool() const NOEXCEPT
    {
        return has_value();
    }

    inline void reset() NOEXCEPT
    {
        inner_.reset();
    }

protected:
    template <typename Type>
    static inline any from_ptr(std::shared_ptr<Type>&& ptr) NOEXCEPT
    {
        // Prevent has_value with null contained pointer.
        any value{};
        if (ptr) value.inner_ = std::forward<std::shared_ptr<Type>>(ptr);
        return value;
    }

    template <typename Type>
    static inline any from_ptr(const std::shared_ptr<Type>& ptr) NOEXCEPT
    {
        // Prevent has_value with null contained pointer.
        any value{};
        if (ptr) value.inner_ = ptr;
        return value;
    }

private:
    template <typename Type>
    inline std::shared_ptr<Type>* get_inner() NOEXCEPT
    {
        return std::any_cast<std::shared_ptr<Type>>(&inner_);
    }

    template <typename Type>
    inline const std::shared_ptr<Type>* get_inner() const NOEXCEPT
    {
        return std::any_cast<const std::shared_ptr<Type>>(&inner_);
    }

    std::any inner_;
};

BC_POP_WARNING()

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
