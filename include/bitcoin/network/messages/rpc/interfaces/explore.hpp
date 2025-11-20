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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACES_EXPLORE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACES_EXPLORE_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/interface.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct explore_methods
{
    static constexpr std::tuple methods
    {
        method<"block", nullable<string_t>, nullable<number_t>>{ "hash", "height" },
        method<"header", nullable<string_t>, nullable<number_t>>{ "hash", "height" },
        method<"filter", nullable<string_t>, nullable<number_t>>{ "hash", "height" },
        method<"block_tx", nullable<string_t>, nullable<number_t>, number_t>{ "hash", "height", "position" },
        method<"block_txs", nullable<string_t>, nullable<number_t>>{ "hash", "height" },
        method<"transaction", string_t>{ "hash" },
        method<"input", string_t, nullable<number_t>>{ "hash", "index" },
        method<"inputs", string_t>{ "hash" },
        method<"input_script", string_t, nullable<number_t>>{ "hash", "index" },
        method<"input_scripts", string_t>{ "hash" },
        method<"input_witness", string_t, nullable<number_t>>{ "hash", "index" },
        method<"input_witnesses", string_t>{ "hash" },
        method<"output", string_t, nullable<number_t>>{ "hash", "index" },
        method<"outputs", string_t>{ "hash" },
        method<"output_script", string_t, nullable<number_t>>{ "hash", "index" },
        method<"output_scripts", string_t>{ "hash" },
        method<"output_spender", string_t, nullable<number_t>>{ "hash", "index" },
        method<"output_spenders", string_t>{ "hash" },
        method<"addresses", string_t>{ "hash" }
    };

    template <typename... Args>
    using subscriber = network::unsubscriber<Args...>;

    template <size_t Index>
    using at = method_at<methods, Index>;

    // Derive this from above in c++26 using reflection.
    using block = at<0>;
    using header = at<1>;
    using filter = at<2>;
    using block_tx = at<3>;
    using block_txs = at<4>;
    using transaction = at<5>;
    using input = at<6>;
    using inputs = at<7>;
    using input_script = at<8>;
    using input_scripts = at<9>;
    using input_witness = at<10>;
    using input_witnesses = at<11>;
    using output = at<12>;
    using outputs = at<13>;
    using output_script = at<14>;
    using output_scripts = at<15>;
    using output_spender = at<16>;
    using output_spenders = at<17>;
    using addresses = at<18>;
};

using interface_explore = interface<explore_methods>;

/// Pagination and filtering are via query string.
enum explore_targets
{
    /// /v[]/block/hash/[bkhash] {1}
    /// /v[]/block/height/[height] {1}s
    block,

    /// /v[]/block/hash/[bkhash]/header {1}
    /// /v[]/block/height/[height]/header {1}
    header,

    /// /v[]/block/hash/[bkhash]/filter {1}
    /// /v[]/block/height/[height]/filter {1}
    filter,

    /// -----------------------------------------------------------------------

    /// /v[]/block/hash/[bkhash]/transaction/[position] {1}
    /// /v[]/block/height/[height]/transaction/[position] {1}
    block_tx,

    /// /v[]/block/hash/[bkhash]/transactions {all txs in the block}
    /// /v[]/block/height/[height]/transactions {all txs in the block}
    block_txs,

    /// /v[]/transaction/hash/[txhash] {1}
    transaction,

    /// -----------------------------------------------------------------------

    /// /v[]/input/[txhash]/[index] {1}
    input,

    /// /v[]/inputs/[txhash] {all inputs in the tx}
    inputs,

    /// /v[]/input/[txhash]/[index]/script {1}
    input_script,

    /// /v[]/input/[txhash]/scripts {all input scripts in the tx}
    input_scripts,

    /// /v[]/input/[txhash]/[index]/witness {1}
    input_witness,

    /// /v[]/input/[txhash]/witnesses {all witnesses in the tx}
    input_witnesses,

    /// -----------------------------------------------------------------------

    /// /v[]/output/[txhash]/[index] {1}
    output,

    /// /v[]/outputs/[txhash] {all outputs in the tx}
    outputs,

    /// /v[]/output/[txhash]/[index]/script {1}
    output_script,

    /// /v[]/output/[txhash]/scripts {all output scripts in the tx}
    output_scripts,

    /// /v[]/output/[txhash]/[index]/spender {1 - confirmed}
    output_spender,

    /// /v[]/output/[txhash]/spenders {all}
    output_spenders,

    /// -----------------------------------------------------------------------

    /// /v[]/address/[output-script-hash] {all}
    address
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
