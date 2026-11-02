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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_VERB_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_ENUMS_VERB_HPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// Request methods (rfc9110 and registered extensions), and websocket and
/// stream, which tag a message received over an upgraded or downgraded
/// connection.
enum class verb : uint8_t
{
    unknown,
    delete_,
    get,
    head,
    post,
    put,
    connect,
    options,
    trace,
    copy,
    lock,
    mkcol,
    move,
    propfind,
    proppatch,
    search,
    unlock,
    bind,
    rebind,
    unbind,
    acl,
    report,
    mkactivity,
    checkout,
    merge,
    msearch,
    notify,
    subscribe,
    unsubscribe,
    patch,
    purge,
    mkcalendar,
    link,
    unlink,
    websocket,
    stream
};

/// The verb of the method token (case sensitive), or unknown.
BCT_API verb to_verb(const std::string_view& method) NOEXCEPT;

/// The method token of the verb, empty for unknown, websocket and stream.
BCT_API std::string_view to_string(verb method) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
