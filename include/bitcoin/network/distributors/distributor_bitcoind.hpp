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
#ifndef LIBBITCOIN_NETWORK_DISTRIBUTORS_DISTRIBUTOR_BITCOIND_HPP
#define LIBBITCOIN_NETWORK_DISTRIBUTORS_DISTRIBUTOR_BITCOIND_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/rpc.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct bitcoind_methods
{
    static constexpr std::tuple methods
    {
        method<"getbestblockhash">{},
        method<"getblockhash", nullable<double>>{ "height" },
        method<"getblock", string_t, optional<1.0>>{ "blockhash", "verbosity" },
        method<"getblockheader", string_t, optional<true>>{ "blockhash", "verbose" },
        method<"getblockstats", string_t, array_t>{ "hash_or_height", "stats" },
        method<"getchaintxstats", optional<42.0>, optional<"hello"_t>>{ "nblocks", "blockhash" },
    };

    // Derive this from above in c++26 using reflection.
    using getbestblockhash = at<0, decltype(methods)>;
    using getblockhash = at<1, decltype(methods)>;
    using getblock = at<2, decltype(methods)>;
    using getblockheader = at<3, decltype(methods)>;
    using getblockstats = at<4, decltype(methods)>;
    using getchaintxstats = at<5, decltype(methods)>;
};

using bitcoind = interface<bitcoind_methods>;

static_assert(bitcoind::getblock::name == "getblock");
static_assert(bitcoind::getblock::size == 2u);

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
