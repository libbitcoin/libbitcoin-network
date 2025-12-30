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
#ifndef LIBBITCOIN_NETWORK_INTERFACES_HTTP_HPP
#define LIBBITCOIN_NETWORK_INTERFACES_HTTP_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/messages.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct http_methods
{
    static constexpr std::tuple methods
    {
        method<"get", http::method::get::cptr>{},
        method<"head", http::method::head::cptr>{},
        method<"post", http::method::post::cptr>{},
        method<"put", http::method::put::cptr>{},
        method<"delete_", http::method::delete_::cptr>{},
        method<"trace", http::method::trace::cptr>{},
        method<"options", http::method::options::cptr>{},
        method<"connect", http::method::connect::cptr>{},
        method<"unknown", http::method::unknown::cptr>{}
    };

    /// Subscriber requires void handlers, injects `code` parameter.
    template <typename... Args>
    using subscriber = network::subscriber<Args...>;

    /// dispatcher.subscribe(std::forward<signature>(handler));
    template <class Request>
    using signature = std::function<void(const code&,
        const typename Request::cptr&)>;
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
