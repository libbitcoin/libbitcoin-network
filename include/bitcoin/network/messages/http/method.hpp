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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_METHOD_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_METHOD_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/verb.hpp>

/// Type-differentiation for request message distribution.

namespace libbitcoin {
namespace network {
namespace http {
namespace method {

struct BCT_API method_ptr
{
    /// Overload structure -> to obtain .ptr.
    const string_request* operator->() const NOEXCEPT
    {
        return ptr.get();
    }

    /// Overload structure * to obtain *ptr.
    const string_request& operator*() const NOEXCEPT
    {
        return *ptr;
    }

    /// Test before pointer dereference.
    operator bool() const NOEXCEPT
    {
        return !!ptr;
    }

    string_request_cptr ptr{};
};

template <verb Verb>
struct method_alias : public method_ptr
{
    /// May differ from from ptr->method() (e.g. verb::unknown).
    static constexpr verb method = Verb;
};

using get     = method_alias<verb::get>;
using head    = method_alias<verb::head>;
using post    = method_alias<verb::post>;
using put     = method_alias<verb::put>;
using delete_ = method_alias<verb::delete_>;
using trace   = method_alias<verb::trace>;
using options = method_alias<verb::options>;
using connect = method_alias<verb::connect>;
using unknown = method_alias<verb::unknown>;

} // namespace method
} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
