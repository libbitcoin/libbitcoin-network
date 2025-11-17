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

////struct bitcoind_methods
////{
////    static constexpr std::tuple methods
////    {
////        // Blockchain RPCs
////        method<"getbestblockhash">{},
////        method<"getblock", string_t, optional<0.0>>{ "blockhash", "verbosity" },
////        method<"getblockchaininfo">{},
////        method<"getblockcount">{},
////        method<"getblockfilter", string_t, optional<"basic"_t>>{ "blockhash", "filtertype" },
////        method<"getblockhash", number_t>{ "height" },
////        method<"getblockheader", string_t, optional<true>>{ "blockhash", "verbose" },
////        method<"getblockstats", string_t, optional<empty::array>>{ "hash_or_height", "stats" },
////        method<"getchaintxstats", optional<-1.0>, optional<""_t>>{ "nblocks", "blockhash" },
////        method<"getchainwork">{},
////        method<"gettxout", string_t, number_t, optional<true>>{ "txid", "n", "include_mempool" },
////        method<"gettxoutsetinfo">{},
////        method<"pruneblockchain", number_t>{ "height" },
////        method<"savemempool">{},
////        method<"scantxoutset", string_t, optional<empty::array>>{ "action", "scanobjects" },
////        method<"verifychain", optional<4.0>, optional<288.0>>{ "checklevel", "nblocks" },
////        method<"verifytxoutset", string_t>{ "input_verify_flag" },
////
////        // Control RPCs
////        method<"getmemoryinfo", optional<"stats"_t>>{ "mode" },
////        method<"getrpcinfo">{},
////        method<"help", optional<""_t>>{ "command" },
////        method<"logging", optional<"*"_t>>{ "include" },
////        method<"stop">{},
////        method<"uptime">{},
////
////        // Mining RPCs
////        method<"getblocktemplate", optional<empty::object>>{ "template_request" },
////        method<"getmininginfo">{},
////        method<"getnetworkhashps", optional<120.0>, optional<-1.0>>{ "nblocks", "height" },
////        method<"prioritisetransaction", string_t, number_t, number_t>{ "txid", "dummy", "priority_delta" },
////        method<"submitblock", string_t, optional<""_t>>{ "block", "parameters" },
////
////        // Network RPCs
////        method<"addnode", string_t, string_t>{ "node", "command" },
////        method<"clearbanned">{},
////        method<"disconnectnode", string_t, optional<-1.0>>{ "address", "nodeid" },
////        method<"getaddednodeinfo", optional<false>, optional<true>, optional<""_t>>{ "include_chain_info", "dns", "addnode" },
////        method<"getconnectioncount">{},
////        method<"getnetworkinfo">{},
////        method<"getpeerinfo">{},
////        method<"listbanned">{},
////        method<"ping">{},
////        method<"setban", string_t, string_t, optional<86400.0>, optional<false>, optional<""_t>>{ "addr", "command", "bantime", "absolute", "reason" },
////        method<"setnetworkactive", boolean_t>{ "state" },
////
////        // Rawtransactions RPCs
////        method<"combinerawtransaction", array_t>{ "txs" },
////        method<"createrawtransaction", array_t, object_t, optional<0.0>, optional<false>>{ "inputs", "outputs", "locktime", "replaceable" },
////        method<"decoderawtransaction", string_t>{ "hexstring" },
////        method<"fundrawtransaction", string_t, optional<empty::object>>{ "rawtx", "options" },
////        method<"getrawtransaction", string_t, optional<0.0>, optional<""_t>>{ "txid", "verbose", "blockhash" },
////        method<"sendrawtransaction", string_t, optional<0.0>>{ "hexstring", "maxfeerate" },
////        method<"signrawtransactionwithkey", string_t, optional<empty::array>, optional<empty::array>, optional<"ALL|FORKID"_t>>{ "hexstring", "privkeys", "prevtxs", "sighashtype" },
////        method<"testmempoolaccept", array_t, optional<0.0>>{ "rawtxs", "maxfeerate" },
////        method<"testrawtransaction", string_t>{ "rawtx" },
////
////        // Util RPCs (node-related)
////        method<"createmultisig", number_t, array_t>{ "nrequired", "keys" },
////        method<"decodepsbt", string_t>{ "psbt" },
////        method<"decodescript", string_t>{ "hex" },
////        method<"estimaterawfee", number_t, optional<"unset"_t>>{ "conf_target", "estimate_mode" },
////        method<"getdescriptorinfo", string_t>{ "descriptor" },
////        method<"validateaddress", string_t>{ "address" }
////    };
////
////    // Derive this from above in c++26 using reflection.
////    using getbestblockhash = at<0, decltype(methods)>;
////    using getblock = at<1, decltype(methods)>;
////    using getblockchaininfo = at<2, decltype(methods)>;
////    using getblockcount = at<3, decltype(methods)>;
////    using getblockfilter = at<4, decltype(methods)>;
////    using getblockhash = at<5, decltype(methods)>;
////    using getblockheader = at<6, decltype(methods)>;
////    using getblockstats = at<7, decltype(methods)>;
////    using getchaintxstats = at<8, decltype(methods)>;
////    using getchainwork = at<9, decltype(methods)>;
////    using gettxout = at<10, decltype(methods)>;
////    using gettxoutsetinfo = at<11, decltype(methods)>;
////    using pruneblockchain = at<12, decltype(methods)>;
////    using savemempool = at<13, decltype(methods)>;
////    using scantxoutset = at<14, decltype(methods)>;
////    using verifychain = at<15, decltype(methods)>;
////    using verifytxoutset = at<16, decltype(methods)>;
////    using getmemoryinfo = at<17, decltype(methods)>;
////    using getrpcinfo = at<18, decltype(methods)>;
////    using help = at<19, decltype(methods)>;
////    using logging = at<20, decltype(methods)>;
////    using stop = at<21, decltype(methods)>;
////    using uptime = at<22, decltype(methods)>;
////    using getblocktemplate = at<23, decltype(methods)>;
////    using getmininginfo = at<24, decltype(methods)>;
////    using getnetworkhashps = at<25, decltype(methods)>;
////    using prioritisetransaction = at<26, decltype(methods)>;
////    using submitblock = at<27, decltype(methods)>;
////    using addnode = at<28, decltype(methods)>;
////    using clearbanned = at<29, decltype(methods)>;
////    using disconnectnode = at<30, decltype(methods)>;
////    using getaddednodeinfo = at<31, decltype(methods)>;
////    using getconnectioncount = at<32, decltype(methods)>;
////    using getnetworkinfo = at<33, decltype(methods)>;
////    using getpeerinfo = at<34, decltype(methods)>;
////    using listbanned = at<35, decltype(methods)>;
////    using ping = at<36, decltype(methods)>;
////    using setban = at<37, decltype(methods)>;
////    using setnetworkactive = at<38, decltype(methods)>;
////    using combinerawtransaction = at<39, decltype(methods)>;
////    using createrawtransaction = at<40, decltype(methods)>;
////    using decoderawtransaction = at<41, decltype(methods)>;
////    using fundrawtransaction = at<42, decltype(methods)>;
////    using getrawtransaction = at<43, decltype(methods)>;
////    using sendrawtransaction = at<44, decltype(methods)>;
////    using signrawtransactionwithkey = at<45, decltype(methods)>;
////    using testmempoolaccept = at<46, decltype(methods)>;
////    using testrawtransaction = at<47, decltype(methods)>;
////    using createmultisig = at<48, decltype(methods)>;
////    using decodepsbt = at<49, decltype(methods)>;
////    using decodescript = at<50, decltype(methods)>;
////    using estimaterawfee = at<51, decltype(methods)>;
////    using getdescriptorinfo = at<52, decltype(methods)>;
////    using validateaddress = at<53, decltype(methods)>;
////};
////
////using bitcoind = interface<bitcoind_methods>;
////
////static_assert(bitcoind::getblock::name == "getblock");
////static_assert(bitcoind::getblock::size == 2u);

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
