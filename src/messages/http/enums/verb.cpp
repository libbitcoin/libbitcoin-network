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
#include <bitcoin/network/messages/http/enums/verb.hpp>

#include <algorithm>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

struct verb_name
{
    verb method;
    std::string_view name;
};

static constexpr std::array<verb_name, 33> verbs
{
    verb_name{ verb::delete_, "DELETE" },
    verb_name{ verb::get, "GET" },
    verb_name{ verb::head, "HEAD" },
    verb_name{ verb::post, "POST" },
    verb_name{ verb::put, "PUT" },
    verb_name{ verb::connect, "CONNECT" },
    verb_name{ verb::options, "OPTIONS" },
    verb_name{ verb::trace, "TRACE" },
    verb_name{ verb::copy, "COPY" },
    verb_name{ verb::lock, "LOCK" },
    verb_name{ verb::mkcol, "MKCOL" },
    verb_name{ verb::move, "MOVE" },
    verb_name{ verb::propfind, "PROPFIND" },
    verb_name{ verb::proppatch, "PROPPATCH" },
    verb_name{ verb::search, "SEARCH" },
    verb_name{ verb::unlock, "UNLOCK" },
    verb_name{ verb::bind, "BIND" },
    verb_name{ verb::rebind, "REBIND" },
    verb_name{ verb::unbind, "UNBIND" },
    verb_name{ verb::acl, "ACL" },
    verb_name{ verb::report, "REPORT" },
    verb_name{ verb::mkactivity, "MKACTIVITY" },
    verb_name{ verb::checkout, "CHECKOUT" },
    verb_name{ verb::merge, "MERGE" },
    verb_name{ verb::msearch, "M-SEARCH" },
    verb_name{ verb::notify, "NOTIFY" },
    verb_name{ verb::subscribe, "SUBSCRIBE" },
    verb_name{ verb::unsubscribe, "UNSUBSCRIBE" },
    verb_name{ verb::patch, "PATCH" },
    verb_name{ verb::purge, "PURGE" },
    verb_name{ verb::mkcalendar, "MKCALENDAR" },
    verb_name{ verb::link, "LINK" },
    verb_name{ verb::unlink, "UNLINK" }
};

verb to_verb(const std::string_view& method) NOEXCEPT
{
    const auto it = std::ranges::find(verbs, method, &verb_name::name);
    return it == verbs.end() ? verb::unknown : it->method;
}

std::string_view to_string(verb method) NOEXCEPT
{
    const auto it = std::ranges::find(verbs, method, &verb_name::method);
    return it == verbs.end() ? std::string_view{} : it->name;
}

} // namespace http
} // namespace network
} // namespace libbitcoin
