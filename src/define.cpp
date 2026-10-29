/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#include <bitcoin/network/define.hpp>

// define.hpp is the common include for /network.
// All non-network headers include define.hpp.
// Network inclusions are chained as follows.

// /ssl           : <nothing>
// version        : <generated>
// have           : version
// preprocessor   : have <bitcoin/system>
// boost          : /ssl preprocessor
// asio           : boost
// beast          : asio
// error          : beast
// define         : error

// Other directory common includes are not internally chained.
// Each header includes only its required common headers.

// memory         : define
// /async         : define
// /messages      : memory                         [interfaces/peer_dispatch.hpp]
// /config        : /messages
// /broadcast     : /async /config
// /interfaces    : /broadcast
// /log           : /async
// /p2ps          : /messages
// /zmtp          : define
// settings       : /async /config /p2ps /zmtp
// /net           : /log settings
// /channels      : /interfaces /net
// /sessions      : /channels                      [forward: net]
// net            : /sessions
// /protocols     : /sessions
// /tls           : define

// When a symbol is unexplainably undefined, its defining include is probably
// creating the circularity.
