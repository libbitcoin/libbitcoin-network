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
#include <bitcoin/network/messages/rpc/size.hpp>

#include <algorithm>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/model.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

using namespace system;

size_t to_size(const value_t& value) NOEXCEPT
{
    return std::visit(overload
    {
        [](const string_t& text) NOEXCEPT
        {
            return text.size() + element_size;
        },
        [](const array_t& values) NOEXCEPT
        {
            return to_size(values);
        },
        [](const object_t& values) NOEXCEPT
        {
            return to_size(values);
        },
        [](const auto&) NOEXCEPT
        {
            return element_size;
        }
    }, value.value());
}

size_t to_size(const array_t& values) NOEXCEPT
{
    return std::accumulate(values.cbegin(), values.cend(), element_size,
        [](size_t total, const value_t& value) NOEXCEPT
        {
            return total + to_size(value);
        });
}

size_t to_size(const object_t& values) NOEXCEPT
{
    return std::accumulate(values.cbegin(), values.cend(), element_size,
        [](size_t total, const auto& value) NOEXCEPT
        {
            return total + value.first.size() + to_size(value.second);
        });
}

size_t to_size(const params_option& params) NOEXCEPT
{
    if (!params)
        return zero;

    return std::visit(overload
    {
        [](const value_t& value) NOEXCEPT
        {
            return to_size(value);
        },
        [](const array_t& values) NOEXCEPT
        {
            return to_size(values);
        },
        [](const object_t& values) NOEXCEPT
        {
            return to_size(values);
        }
    }, *params);
}

size_t to_size(const response_t& message) NOEXCEPT
{
    const auto result = message.result ? to_size(*message.result) : zero;
    const auto error = message.error ? message.error->message.size() +
        element_size + (message.error->data ? to_size(*message.error->data) :
            zero) : zero;

    return envelope_size + result + error;
}

size_t to_size(const request_t& message) NOEXCEPT
{
    return envelope_size + message.method.size() + to_size(message.params);
}

} // namespace rpc
} // namespace network
} // namespace libbitcoin
