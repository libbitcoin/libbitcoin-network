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
#include <bitcoin/network/ws/handshake.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/fields.hpp>

namespace libbitcoin {
namespace network {
namespace ws {

using namespace system;
using namespace network::http;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// The base64 encoding of a 16 byte nonce.
constexpr size_t maximum_key = 24;

static code reject(response& out, const request_header& request,
    const code& reason) NOEXCEPT
{
    out = response{ status::bad_request, request.version() };
    out.body() = reason.message();
    out.content_length(out.body().size());
    return reason;
}

code accept(response& out, const request_header& request) NOEXCEPT
{
    if (request.version() != version_1_1)
        return reject(out, request, error::bad_http_version);

    if (request.method() != verb::get)
        return reject(out, request, error::websocket_bad_method);

    if (is_zero(request.count(field::host)))
        return reject(out, request, error::no_host);

    const auto connection = request.find(field::connection);
    if (connection == request.end())
        return reject(out, request, error::no_connection);

    if (!has_token(connection->value, "upgrade"))
        return reject(out, request, error::no_connection_upgrade);

    const auto upgrade = request.find(field::upgrade);
    if (upgrade == request.end())
        return reject(out, request, error::no_upgrade);

    if (!has_token(upgrade->value, "websocket"))
        return reject(out, request, error::no_upgrade_websocket);

    const auto key = request.find(field::sec_websocket_key);
    if (key == request.end())
        return reject(out, request, error::no_sec_key);

    if (key->value.size() > maximum_key)
        return reject(out, request, error::bad_sec_key);

    const auto version = request.find(field::sec_websocket_version);
    if (version == request.end())
        return reject(out, request, error::no_sec_version);

    if (version->value != "13")
    {
        out = response{ status::upgrade_required, request.version() };
        out.set(field::sec_websocket_version, "13");
        out.content_length(zero);
        return error::bad_sec_version;
    }

    out = response{ status::switching_protocols, request.version() };
    out.set(field::upgrade, "websocket");
    out.set(field::connection, "Upgrade");
    out.set(field::sec_websocket_accept, to_websocket_accept(request));
    return error::success;
}

BC_POP_WARNING()

} // namespace ws
} // namespace network
} // namespace libbitcoin
