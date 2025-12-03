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
#ifndef LIBBITCOIN_NETWORK_RPC_INTERFACES_EXPLORE_HPP
#define LIBBITCOIN_NETWORK_RPC_INTERFACES_EXPLORE_HPP

#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/rpc/publish.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct explore_methods
{
    static constexpr std::tuple methods
    {
        method<"block", uint8_t, uint8_t, nullable<system::hash_cptr>, nullable<uint32_t>, optional<true>>{ "version", "media", "hash", "height", "witness" },
        method<"header", uint8_t, uint8_t, nullable<system::hash_cptr>, nullable<uint32_t>>{ "version", "media", "hash", "height" },
        method<"block_txs", uint8_t, uint8_t, nullable<system::hash_cptr>, nullable<uint32_t>>{ "version", "media", "hash", "height" },
        method<"block_tx", uint8_t, uint8_t, uint32_t, nullable<system::hash_cptr>, nullable<uint32_t>, optional<true>>{ "version", "media", "position", "hash", "height", "witness" },
        method<"transaction", uint8_t, uint8_t, system::hash_cptr, optional<true>>{ "version", "media", "hash", "witness" },
        method<"tx_block", uint8_t, uint8_t, system::hash_cptr>{ "version", "media", "hash" },

        method<"inputs", uint8_t, uint8_t, system::hash_cptr, optional<true>>{ "version", "media", "hash", "witness" },
        method<"input", uint8_t, uint8_t, system::hash_cptr, uint32_t, optional<true>>{ "version", "media", "hash", "index", "witness" },
        method<"input_script", uint8_t, uint8_t, system::hash_cptr, uint32_t>{ "version", "media", "hash", "index" },
        method<"input_witness", uint8_t, uint8_t, system::hash_cptr, uint32_t>{ "version", "media", "hash", "index" },
 
        method<"outputs", uint8_t, uint8_t, system::hash_cptr>{ "version", "media", "hash" },
        method<"output", uint8_t, uint8_t, system::hash_cptr, uint32_t>{ "version", "media", "hash", "index" },
        method<"output_script", uint8_t, uint8_t, system::hash_cptr, uint32_t>{ "version", "media", "hash", "index" },
        method<"output_spender", uint8_t, uint8_t, system::hash_cptr, uint32_t>{ "version", "media", "hash", "index" },
        method<"output_spenders", uint8_t, uint8_t, system::hash_cptr, uint32_t>{ "version", "media", "hash", "index" },

        method<"address", uint8_t, uint8_t, system::hash_cptr>{ "version", "media", "hash" },
        method<"filter", uint8_t, uint8_t, uint8_t, nullable<system::hash_cptr>, nullable<uint32_t>>{ "version", "media", "type", "hash", "height" },
        method<"filter_hash", uint8_t, uint8_t, uint8_t, nullable<system::hash_cptr>, nullable<uint32_t>>{ "version", "media", "type", "hash", "height" },
        method<"filter_header", uint8_t, uint8_t, uint8_t, nullable<system::hash_cptr>, nullable<uint32_t>>{ "version", "media", "type", "hash", "height" }
    };

    template <typename... Args>
    using subscriber = network::unsubscriber<Args...>;

    template <size_t Index>
    using at = method_at<methods, Index>;

    // Derive this from above in c++26 using reflection.
    using block = at<0>;
    using header = at<1>;
    using block_txs = at<2>;
    using block_tx = at<3>;
    using transaction = at<4>;
    using tx_block = at<5>;

    using inputs = at<6>;
    using input = at<7>;
    using input_script = at<8>;
    using input_witness = at<9>;

    using outputs = at<10>;
    using output = at<11>;
    using output_script = at<12>;
    using output_spender = at<13>;
    using output_spenders = at<14>;

    using address = at<15>;
    using filter = at<16>;
    using filter_hash = at<17>;
    using filter_header = at<18>;
};

/// Pagination and filtering are via query string.
enum explore_targets
{
    /// /v[]/block/hash/[bkhash] {1}
    /// /v[]/block/height/[height] {1}
    block,

    /// /v[]/block/hash/[bkhash]/header {1}
    /// /v[]/block/height/[height]/header {1}
    header,

    /////// /v[]/block/hash/[bkhash]/context {1}
    /////// /v[]/block/height/[height]/context {1}
    ////context,

    /// -----------------------------------------------------------------------

    /// /v[]/block/hash/[bkhash]/transactions {all txs in the block}
    /// /v[]/block/height/[height]/transactions {all txs in the block}
    block_txs,

    /// /v[]/block/hash/[bkhash]/transaction/[position] {1}
    /// /v[]/block/height/[height]/transaction/[position] {1}
    block_tx,

    /// /v[]/transaction/[txhash] {1}
    transaction,

    /// /v[]/transaction/[txhash]/block {1 - confirmed}
    tx_block,

    /// -----------------------------------------------------------------------

    /// /v[]/inputs/[txhash] {all inputs in the tx}
    inputs,

    /// /v[]/input/[txhash]/[index] {1}
    input,

    /// /v[]/input/[txhash]/[index]/script {1}
    input_script,

    /// /v[]/input/[txhash]/[index]/witness {1}
    input_witness,

    /// -----------------------------------------------------------------------

    /// /v[]/outputs/[txhash] {all outputs in the tx}
    outputs,

    /// /v[]/output/[txhash]/[index] {1}
    output,

    /// /v[]/output/[txhash]/[index]/script {1}
    output_script,

    /// /v[]/output/[txhash]/[index]/spender {1 - confirmed}
    output_spender,

    /// /v[]/output/[txhash]/[index]/spenders {all}
    output_spenders,

    /// -----------------------------------------------------------------------

    /// /v[]/address/[output-script-hash] {all}
    address,

    /// /v[]/block/hash/[bkhash]/filter/[type] {1}
    /// /v[]/block/height/[height]/filter/[type] {1}
    filter,

    /// /v[]/block/hash/[bkhash]/filter/[type]/hash {1}
    /// /v[]/block/height/[height]/filter/[type]/hash {1}
    filter_hash,

    /// /v[]/block/hash/[bkhash]/filter/[type]/header {1}
    /// /v[]/block/height/[height]/filter/[type]/header {1}
    filter_header
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
