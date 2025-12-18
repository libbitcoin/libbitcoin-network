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

#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/verb.hpp>
#include <bitcoin/network/messages/monad_body.hpp>

/// Type-differentiation for request message distribution.

namespace libbitcoin {
namespace network {
namespace http {
namespace method {

template <verb Verb>
struct tagged_request
  : public http::request
{
    using cptr = std::shared_ptr<const tagged_request<Verb>>;
    static constexpr verb method = Verb;
};

using get     = tagged_request<verb::get>;
using post    = tagged_request<verb::post>;
using put     = tagged_request<verb::put>;
using delete_ = tagged_request<verb::delete_>;
using head    = tagged_request<verb::head>;
using options = tagged_request<verb::options>;
using trace   = tagged_request<verb::trace>;
using connect = tagged_request<verb::connect>;
using unknown = tagged_request<verb::unknown>;

template <verb Verb>
inline auto tag_request(const http::request_cptr& request) NOEXCEPT
{
    return typename tagged_request<Verb>::cptr
    {
        request, system::pointer_cast<const tagged_request<Verb>>(request.get())
    };
}

} // namespace method
} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
