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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_MONAD_REQUEST_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_MONAD_REQUEST_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/monad/body.hpp>
#include <bitcoin/network/messages/monad/head.hpp>

namespace libbitcoin {
namespace network {
namespace monad {

struct request
{
    monad::head<true> head{};
    monad::body body{};
};

} // namespace monad
} // namespace network
} // namespace libbitcoin

// TODO: move to monad namespace.
namespace libbitcoin {
namespace network {
namespace http {
    
// TODO: move to monad::request.
using request = boost::beast::http::request<monad::body>;
using parser = boost::beast::http::request_parser<monad::body>;
using request_cptr = std::shared_ptr<const request>;
using request_ptr = std::shared_ptr<request>;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
