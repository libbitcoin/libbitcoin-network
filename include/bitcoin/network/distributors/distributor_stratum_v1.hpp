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
#ifndef LIBBITCOIN_NETWORK_DISTRIBUTORS_DISTRIBUTOR_STRATUM_V1_HPP
#define LIBBITCOIN_NETWORK_DISTRIBUTORS_DISTRIBUTOR_STRATUM_V1_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/rpc.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct stratum_v1_methods
{
    static constexpr std::tuple methods
    {
        // Client requests.
        method<"mining.subscribe", optional<""_t>, optional<0>>{ "user_agent", "extranonce1_size" },
        method<"mining.authorize", string_t, string_t>{ "username", "password" },
        method<"mining.submit", string_t, string_t, string_t, number_t, string_t>{ "worker_name", "job_id", "extranonce2", "ntime", "nonce" },
        method<"mining.extranonce.subscribe">{},
        method<"mining.extranonce.unsubscribe", number_t>{ "id" },

        // Server notifications.
        method<"mining.configure", object_t>{ "extensions" },
        method<"mining.set_difficulty", optional<1.0>>{ "difficulty" },
        method<"mining.notify", string_t, string_t, string_t, string_t, array_t, number_t, number_t, number_t, boolean_t, boolean_t, boolean_t>{ "job_id", "prevhash", "coinb1", "coinb2", "merkle_branch", "version", "nbits", "ntime", "clean_jobs", "hash1", "hash2" },
        method<"client.reconnect", string_t, number_t, number_t>{ "url", "port", "id" },
        method<"client.hello", object_t>{ "protocol" },
        method<"client.rejected", string_t, string_t>{ "job_id", "reject_reason" }
    };

    using mining_subscribe = at<0, decltype(methods)>;
    using mining_authorize = at<1, decltype(methods)>;
    using mining_submit = at<2, decltype(methods)>;
    using mining_extranonce_subscribe = at<3, decltype(methods)>;
    using mining_extranonce_unsubscribe = at<4, decltype(methods)>;
    using mining_configure = at<5, decltype(methods)>;
    using mining_set_difficulty = at<6, decltype(methods)>;
    using mining_notify = at<7, decltype(methods)>;
    using client_reconnect = at<8, decltype(methods)>;
    using client_hello = at<9, decltype(methods)>;
    using client_rejected = at<10, decltype(methods)>;
};

using stratum_v1 = interface<stratum_v1_methods>;

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
