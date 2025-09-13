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

#include <bitcoin/system.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/messages/rpc/messages.hpp>

namespace libbitcoin {
namespace network {

// Compiler can't see is_null(arena).
BC_PUSH_WARNING(NO_UNGUARDED_POINTERS)

using namespace system;

#define SUBSCRIBER(name) name##_subscriber_
#define MAKE_SUBSCRIBER(name) SUBSCRIBER(name)(strand)
#define STOP_SUBSCRIBER(name) SUBSCRIBER(name).stop_default(ec)
#define CASE_NOTIFY(name) case messages::rpc::identifier::name: \
    return do_notify<messages::rpc::name>(SUBSCRIBER(name), data)

distributor_client::distributor_client(asio::strand& strand) NOEXCEPT
  : MAKE_SUBSCRIBER(ping)
{
}

code distributor_client::notify(messages::rpc::identifier id,
    const data_chunk& data) NOEXCEPT
{
    switch (id)
    {
        CASE_NOTIFY(ping);
        case messages::rpc::identifier::unknown:
        default:
            return error::unknown_message;
    }
}

void distributor_client::stop(const code& ec) NOEXCEPT
{
    STOP_SUBSCRIBER(ping);
}

BC_POP_WARNING()

#undef SUBSCRIBER
#undef MAKE_SUBSCRIBER
#undef CASE_NOTIFY
#undef STOP_SUBSCRIBER

} // namespace network
} // namespace libbitcoin
