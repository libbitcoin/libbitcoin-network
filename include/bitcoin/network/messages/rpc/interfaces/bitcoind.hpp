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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACES_BITCOIND_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_INTERFACES_BITCOIND_HPP

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/rpc/interface.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

struct bitcoind_methods
{
    static constexpr std::tuple methods
    {
        // Blockchain methods.
        method<"getbestblockhash">{},
        method<"getblock", string_t, optional<0>>{ "blockhash", "verbosity" },
        method<"getblockchaininfo">{},
        method<"getblockcount">{},
        method<"getblockfilter", string_t, optional<"basic"_t>>{ "blockhash", "filtertype" },
        method<"getblockhash", number_t>{ "height" },
        method<"getblockheader", string_t, optional<true>>{ "blockhash", "verbose" },
        method<"getblockstats", string_t, optional<empty::array>>{ "hash_or_height", "stats" },
        method<"getchaintxstats", optional<-1>, optional<""_t>>{ "nblocks", "blockhash" },
        method<"getchainwork">{},
        method<"gettxout", string_t, number_t, optional<true>>{ "txid", "n", "include_mempool" },
        method<"gettxoutsetinfo">{},
        method<"pruneblockchain", number_t>{ "height" },
        method<"savemempool">{},
        method<"scantxoutset", string_t, optional<empty::array>>{ "action", "scanobjects" },
        method<"verifychain", optional<4>, optional<288>>{ "checklevel", "nblocks" },
        method<"verifytxoutset", string_t>{ "input_verify_flag" },

        // Control methods.
        method<"getmemoryinfo", optional<"stats"_t>>{ "mode" },
        method<"getrpcinfo">{},
        method<"help", optional<""_t>>{ "command" },
        method<"logging", optional<"*"_t>>{ "include" },
        method<"stop">{},
        method<"uptime">{},

        // Mining methods.
        method<"getblocktemplate", optional<empty::object>>{ "template_request" },
        method<"getmininginfo">{},
        method<"getnetworkhashps", optional<120>, optional<-1>>{ "nblocks", "height" },
        method<"prioritisetransaction", string_t, number_t, number_t>{ "txid", "dummy", "priority_delta" },
        method<"submitblock", string_t, optional<""_t>>{ "block", "parameters" },

        // Network methods.
        method<"addnode", string_t, string_t>{ "node", "command" },
        method<"clearbanned">{},
        method<"disconnectnode", string_t, optional<-1>>{ "address", "nodeid" },
        method<"getaddednodeinfo", optional<false>, optional<true>, optional<""_t>>{ "include_chain_info", "dns", "addnode" },
        method<"getconnectioncount">{},
        method<"getnetworkinfo">{},
        method<"getpeerinfo">{},
        method<"listbanned">{},
        method<"ping">{},
        method<"setban", string_t, string_t, optional<86400>, optional<false>, optional<""_t>>{ "addr", "command", "bantime", "absolute", "reason" },
        method<"setnetworkactive", boolean_t>{ "state" },

        // Rawtransactions methods.
        method<"combinerawtransaction", array_t>{ "txs" },
        method<"createrawtransaction", array_t, object_t, optional<0>, optional<false>>{ "inputs", "outputs", "locktime", "replaceable" },
        method<"decoderawtransaction", string_t>{ "hexstring" },
        method<"fundrawtransaction", string_t, optional<empty::object>>{ "rawtx", "options" },
        method<"getrawtransaction", string_t, optional<0>, optional<""_t>>{ "txid", "verbose", "blockhash" },
        method<"sendrawtransaction", string_t, optional<0>>{ "hexstring", "maxfeerate" },
        method<"signrawtransactionwithkey", string_t, optional<empty::array>, optional<empty::array>, optional<"ALL|FORKID"_t>>{ "hexstring", "privkeys", "prevtxs", "sighashtype" },
        method<"testmempoolaccept", array_t, optional<0>>{ "rawtxs", "maxfeerate" },
        method<"testrawtransaction", string_t>{ "rawtx" },

        // Util methods (node-related).
        method<"createmultisig", number_t, array_t>{ "nrequired", "keys" },
        method<"decodepsbt", string_t>{ "psbt" },
        method<"decodescript", string_t>{ "hex" },
        method<"estimaterawfee", number_t, optional<"unset"_t>>{ "conf_target", "estimate_mode" },
        method<"getdescriptorinfo", string_t>{ "descriptor" },
        method<"validateaddress", string_t>{ "address" },

        // Wallet methods (unsupported).
        method<"abandontransaction", string_t>{ "txid" },
        method<"addmultisigaddress", number_t, array_t, optional<""_t>, optional<"legacy"_t>>{ "nrequired", "keys", "label", "address_type" },
        method<"backupwallet", string_t>{ "destination" },
        method<"bumpfee", string_t, optional<empty::object>>{ "txid", "options" },
        method<"createwallet", string_t, optional<false>, optional<false>, optional<false>, optional<""_t>, optional<false>, optional<false>, optional<false>, optional<"set"_t>>{ "wallet_name", "disable_private_keys", "blank", "passphrase", "avoid_reuse", "descriptors", "load_on_startup", "external_signer", "change_type" },
        method<"dumpprivkey", string_t>{ "address" },
        method<"dumpwallet", string_t>{ "filename" },
        method<"encryptwallet", string_t>{ "passphrase" },
        method<"getaddressinfo", string_t>{ "address" },
        method<"getbalances">{},
        method<"getnewaddress", optional<""_t>, optional<"legacy"_t>>{ "label", "address_type" },
        method<"getreceivedbyaddress", string_t, optional<0>>{ "address", "minconf" },
        method<"getreceivedbylabel", string_t, optional<0>>{ "label", "minconf" },
        method<"gettransaction", string_t, optional<false>, optional<false>>{ "txid", "include_watchonly", "verbose" },
        method<"getunconfirmedbalance">{},
        method<"getwalletinfo">{},
        method<"importaddress", string_t, optional<""_t>, optional<false>, optional<true>>{ "address", "label", "rescan", "p2sh" },
        method<"importmulti", array_t, optional<empty::object>>{ "requests", "options" },
        method<"importprivkey", string_t, optional<""_t>, optional<false>>{ "privkey", "label", "rescan" },
        method<"importprunedfunds", string_t, string_t>{ "rawtransaction", "txoutproof" },
        method<"importpubkey", string_t, optional<""_t>, optional<false>>{ "pubkey", "label", "rescan" },
        method<"importwallet", string_t>{ "filename" },
        method<"keypoolrefill", optional<100>>{ "newsize" },
        method<"listaddressgroupings", optional<1>, optional<false>>{ "minconf", "include_watchonly" },
        method<"listlabels", optional<"receive"_t>>{ "purpose" },
        method<"listlockunspent">{},
        method<"listreceivedbyaddress", optional<1>, optional<false>, optional<false>, optional<""_t>>{ "minconf", "include_empty", "include_watchonly", "address_filter" },
        method<"listreceivedbylabel", optional<1>, optional<false>, optional<false>>{ "minconf", "include_empty", "include_watchonly" },
        method<"listtransactions", optional<""_t>, optional<10>, optional<0>, optional<false>>{ "label", "count", "skip", "include_watchonly" },
        method<"listunspent", optional<1>, optional<empty::array>, optional<true>, optional<false>>{ "minconf", "addresses", "include_unsafe", "query_options" },
        method<"loadwallet", string_t, optional<false>>{ "filename", "load_on_startup" },
        method<"lockunspent", boolean_t, optional<empty::array>>{ "unlock", "transactions" },
        method<"removeprunedfunds", string_t>{ "txid" },
        method<"rescanblockchain", optional<0>>{ "start_height" },
        method<"send", object_t, optional<empty::object>>{ "outputs", "options" },
        method<"sendmany", string_t, object_t, optional<1>, optional<""_t>, optional<""_t>, optional<false>, optional<false>, optional<25>, optional<"unset"_t>, optional<false>, optional<0>>{ "dummy", "outputs", "minconf", "comment", "comment_to", "subtractfeefrom", "replaceable", "conf_target", "estimate_mode", "avoid_reuse", "fee_rate" },
        method<"sendtoaddress", string_t, number_t, optional<""_t>, optional<""_t>, optional<false>, optional<25>, optional<"unset"_t>, optional<false>, optional<0>, optional<false>>{ "address", "amount", "comment", "comment_to", "subtractfeefromamount", "conf_target", "estimate_mode", "avoid_reuse", "fee_rate", "verbose" },
        method<"setlabel", string_t, string_t>{ "address", "label" },
        method<"settxfee", number_t>{ "amount" },
        method<"signmessage", string_t, string_t>{ "address", "message" },
        method<"signmessagewithprivkey", string_t, string_t>{ "privkey", "message" },
        method<"syncwithvalidationinterfacequeue">{},
        method<"unloadwallet", optional<""_t>, optional<false>>{ "wallet_name", "load_on_startup" },
        method<"walletcreatefundedpsbt", optional<empty::array>, optional<empty::object>, optional<0>, optional<empty::object>>{ "inputs", "outputs", "locktime", "options" },
        method<"walletlock">{},
        method<"walletpassphrase", string_t, number_t>{ "passphrase", "timeout" },
        method<"walletprocesspsbt", string_t, optional<false>, optional<false>, optional<false>>{ "psbt", "sign", "bip32derivs", "complete" }
    };

    // Derive this from above in c++26 using reflection.
    // node (supported)
    using getbestblockhash = at<0, decltype(methods)>;
    using getblock = at<1, decltype(methods)>;
    using getblockchaininfo = at<2, decltype(methods)>;
    using getblockcount = at<3, decltype(methods)>;
    using getblockfilter = at<4, decltype(methods)>;
    using getblockhash = at<5, decltype(methods)>;
    using getblockheader = at<6, decltype(methods)>;
    using getblockstats = at<7, decltype(methods)>;
    using getchaintxstats = at<8, decltype(methods)>;
    using getchainwork = at<9, decltype(methods)>;
    using gettxout = at<10, decltype(methods)>;
    using gettxoutsetinfo = at<11, decltype(methods)>;
    using pruneblockchain = at<12, decltype(methods)>;
    using savemempool = at<13, decltype(methods)>;
    using scantxoutset = at<14, decltype(methods)>;
    using verifychain = at<15, decltype(methods)>;
    using verifytxoutset = at<16, decltype(methods)>;
    using getmemoryinfo = at<17, decltype(methods)>;
    using getrpcinfo = at<18, decltype(methods)>;
    using help = at<19, decltype(methods)>;
    using logging = at<20, decltype(methods)>;
    using stop = at<21, decltype(methods)>;
    using uptime = at<22, decltype(methods)>;
    using getblocktemplate = at<23, decltype(methods)>;
    using getmininginfo = at<24, decltype(methods)>;
    using getnetworkhashps = at<25, decltype(methods)>;
    using prioritisetransaction = at<26, decltype(methods)>;
    using submitblock = at<27, decltype(methods)>;
    using addnode = at<28, decltype(methods)>;
    using clearbanned = at<29, decltype(methods)>;
    using disconnectnode = at<30, decltype(methods)>;
    using getaddednodeinfo = at<31, decltype(methods)>;
    using getconnectioncount = at<32, decltype(methods)>;
    using getnetworkinfo = at<33, decltype(methods)>;
    using getpeerinfo = at<34, decltype(methods)>;
    using listbanned = at<35, decltype(methods)>;
    using ping = at<36, decltype(methods)>;
    using setban = at<37, decltype(methods)>;
    using setnetworkactive = at<38, decltype(methods)>;
    using combinerawtransaction = at<39, decltype(methods)>;
    using createrawtransaction = at<40, decltype(methods)>;
    using decoderawtransaction = at<41, decltype(methods)>;
    using fundrawtransaction = at<42, decltype(methods)>;
    using getrawtransaction = at<43, decltype(methods)>;
    using sendrawtransaction = at<44, decltype(methods)>;
    using signrawtransactionwithkey = at<45, decltype(methods)>;
    using testmempoolaccept = at<46, decltype(methods)>;
    using testrawtransaction = at<47, decltype(methods)>;
    using createmultisig = at<48, decltype(methods)>;
    using decodepsbt = at<49, decltype(methods)>;
    using decodescript = at<50, decltype(methods)>;
    using estimaterawfee = at<51, decltype(methods)>;
    using getdescriptorinfo = at<52, decltype(methods)>;
    using validateaddress = at<53, decltype(methods)>;

    // wallet (unsupported)
    using abandontransaction = at<54, decltype(methods)>;
    using addmultisigaddress = at<55, decltype(methods)>;
    using backupwallet = at<56, decltype(methods)>;
    using bumpfee = at<57, decltype(methods)>;
    using createwallet = at<58, decltype(methods)>;
    using dumpprivkey = at<59, decltype(methods)>;
    using dumpwallet = at<60, decltype(methods)>;
    using encryptwallet = at<61, decltype(methods)>;
    using getaddressinfo = at<62, decltype(methods)>;
    using getbalances = at<63, decltype(methods)>;
    using getnewaddress = at<64, decltype(methods)>;
    using getreceivedbyaddress = at<65, decltype(methods)>;
    using getreceivedbylabel = at<66, decltype(methods)>;
    using gettransaction = at<67, decltype(methods)>;
    using getunconfirmedbalance = at<68, decltype(methods)>;
    using getwalletinfo = at<69, decltype(methods)>;
    using importaddress = at<70, decltype(methods)>;
    using importmulti = at<71, decltype(methods)>;
    using importprivkey = at<72, decltype(methods)>;
    using importprunedfunds = at<73, decltype(methods)>;
    using importpubkey = at<74, decltype(methods)>;
    using importwallet = at<75, decltype(methods)>;
    using keypoolrefill = at<76, decltype(methods)>;
    using listaddressgroupings = at<77, decltype(methods)>;
    using listlabels = at<78, decltype(methods)>;
    using listlockunspent = at<79, decltype(methods)>;
    using listreceivedbyaddress = at<80, decltype(methods)>;
    using listreceivedbylabel = at<81, decltype(methods)>;
    using listtransactions = at<82, decltype(methods)>;
    using listunspent = at<83, decltype(methods)>;
    using loadwallet = at<84, decltype(methods)>;
    using lockunspent = at<85, decltype(methods)>;
    using removeprunedfunds = at<86, decltype(methods)>;
    using rescanblockchain = at<87, decltype(methods)>;
    using send = at<88, decltype(methods)>;
    using sendmany = at<89, decltype(methods)>;
    using sendtoaddress = at<90, decltype(methods)>;
    using setlabel = at<91, decltype(methods)>;
    using settxfee = at<92, decltype(methods)>;
    using signmessage = at<93, decltype(methods)>;
    using signmessagewithprivkey = at<94, decltype(methods)>;
    using syncwithvalidationinterfacequeue = at<95, decltype(methods)>;
    using unloadwallet = at<96, decltype(methods)>;
    using walletcreatefundedpsbt = at<97, decltype(methods)>;
    using walletlock = at<98, decltype(methods)>;
    using walletpassphrase = at<99, decltype(methods)>;
    using walletprocesspsbt = at<100, decltype(methods)>;
};

using bitcoind = interface<bitcoind_methods>;

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
