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

struct method_ptr
{
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


template <http::verb Verb>
struct method_alias : public method_ptr
{
    /// May differ from from ptr->method() (e.g. verb::unknown).
    static constexpr http::verb method = Verb;
};

using get     = method_alias<http::verb::get>;
using head    = method_alias<http::verb::head>;
using post    = method_alias<http::verb::post>;
using put     = method_alias<http::verb::put>;
using delete_ = method_alias<http::verb::delete_>;
using trace   = method_alias<http::verb::trace>;
using options = method_alias<http::verb::options>;
using connect = method_alias<http::verb::connect>;
using unknown = method_alias<http::verb::unknown>;

} // namespace method
} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
