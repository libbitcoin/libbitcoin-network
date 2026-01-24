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
#ifndef LIBBITCOIN_NETWORK_DEFINE_HPP
#define LIBBITCOIN_NETWORK_DEFINE_HPP

#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {

#define BIND_SHARED(method, args) \
    std::bind(std::forward<Method>(method), shared_from_base<Derived>(), \
        std::forward<Args>(args)...)
#define BIND_THIS(method, args) \
    std::bind(std::forward<Method>(method), static_cast<Derived*>(this), \
        std::forward<Args>(args)...)
#define BIND(method, ...) \
    bind<CLASS>(&CLASS::method, __VA_ARGS__)
#define POST(method, ...) \
    post<CLASS>(&CLASS::method, __VA_ARGS__)
#define PARALLEL(method, ...) \
    parallel<CLASS>(&CLASS::method, __VA_ARGS__)

} // namespace network
} // namespace libbitcoin

// define.hpp is the common include for /network (except net and settings).
// All non-network headers include define.hpp.
// Network inclusions are chained as follows.

// version        : <generated>
// have           : version
// preprocessor   : have
// boost          : preprocessor
// asio           : boost
// beast          : asio
// error          : beast
// define         : error

// Root directory singletons.

// memory         : define
// settings       : define /config
// net            : define settings /sessions

// Other directory common includes are not internally chained.
// Each header includes only its required common headers.
// protcols are not included in any headers except protocols.

// /ssl           : <nothing>
// /async         : define
// /log           : define /async
// /messages      : define memory /async
// /config        : define /messages
// /interface     : define /messages
// /net           : define settings memory /config /log
// /channels      : define /net /interface
// /sessions      : define /channels
// /protocols     : define /sessions

#endif
