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
#include <bitcoin/network/net/distributor_client.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>

namespace libbitcoin {
namespace network {

using namespace system;

#define SUBSCRIBER(name) name##_subscriber_
#define MAKE_SUBSCRIBER(name) SUBSCRIBER(name)(strand)
#define STOP_SUBSCRIBER(name) SUBSCRIBER(name).stop_default(ec)

#define CASE_NOTIFY(name) \
case http::verb::name: \
return do_notify(SUBSCRIBER(name), messages::rpc::method::name{ request })

distributor_client::distributor_client(asio::strand& strand) NOEXCEPT
  : MAKE_SUBSCRIBER(get),
    MAKE_SUBSCRIBER(head),
    MAKE_SUBSCRIBER(post),
    MAKE_SUBSCRIBER(put),
    MAKE_SUBSCRIBER(delete_),
    MAKE_SUBSCRIBER(trace),
    MAKE_SUBSCRIBER(options),
    MAKE_SUBSCRIBER(connect)
{
}

void distributor_client::notify(
    const http_string_request_cptr& request) const NOEXCEPT
{
    // Does not throw.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    switch (request->method())
    BC_POP_WARNING()
    {
        CASE_NOTIFY(get);
        CASE_NOTIFY(head);
        CASE_NOTIFY(post);
        CASE_NOTIFY(put);
        CASE_NOTIFY(delete_);
        CASE_NOTIFY(trace);
        CASE_NOTIFY(options);
        CASE_NOTIFY(connect);
    }
}

void distributor_client::stop(const code& ec) NOEXCEPT
{
    STOP_SUBSCRIBER(get);
    STOP_SUBSCRIBER(head);
    STOP_SUBSCRIBER(post);
    STOP_SUBSCRIBER(put);
    STOP_SUBSCRIBER(delete_);
    STOP_SUBSCRIBER(trace);
    STOP_SUBSCRIBER(options);
    STOP_SUBSCRIBER(connect);
}

#undef SUBSCRIBER
#undef MAKE_SUBSCRIBER
#undef CASE_NOTIFY
#undef STOP_SUBSCRIBER

} // namespace network
} // namespace libbitcoin
