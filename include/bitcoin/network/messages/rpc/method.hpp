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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_METHOD_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_METHOD_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/async/async.hpp>

 /// Type-differentiation for request message distribution.

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {
namespace method {

template <http::verb Verb>
struct method_
{
    /// May differ from from ptr->method() (e.g. verb::unknown).
    static constexpr http::verb method = Verb;

    /// Overload structure -> to obtain .ptr.
    const http_string_request* operator->() const NOEXCEPT
    {
        return ptr.get();
    }

    /// Overload structure * to obtain *ptr.
    const http_string_request& operator*() const NOEXCEPT
    {
        return *ptr;
    }

    /// Test before pointer dereference.
    operator bool() const NOEXCEPT
    {
        return !!ptr;
    }

    http_string_request_cptr ptr{};
};

using get     = method_<http::verb::get>;
using head    = method_<http::verb::head>;
using post    = method_<http::verb::post>;
using put     = method_<http::verb::put>;
using delete_ = method_<http::verb::delete_>;
using trace   = method_<http::verb::trace>;
using options = method_<http::verb::options>;
using connect = method_<http::verb::connect>;
using unknown = method_<http::verb::unknown>;

} // namespace method
} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
