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
#include <bitcoin/network/messages/http/enums/field.hpp>

#include <algorithm>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/fields.hpp>

namespace libbitcoin {
namespace network {
namespace http {

struct field_name
{
    field name;
    std::string_view text;
};

static constexpr std::array<field_name, 27> field_names
{
    field_name{ field::accept, "Accept" },
    field_name{ field::access_control_allow_headers, "Access-Control-Allow-Headers" },
    field_name{ field::access_control_allow_methods, "Access-Control-Allow-Methods" },
    field_name{ field::access_control_allow_origin, "Access-Control-Allow-Origin" },
    field_name{ field::access_control_max_age, "Access-Control-Max-Age" },
    field_name{ field::access_control_request_headers, "Access-Control-Request-Headers" },
    field_name{ field::access_control_request_method, "Access-Control-Request-Method" },
    field_name{ field::authorization, "Authorization" },
    field_name{ field::connection, "Connection" },
    field_name{ field::content_disposition, "Content-Disposition" },
    field_name{ field::content_length, "Content-Length" },
    field_name{ field::content_type, "Content-Type" },
    field_name{ field::date, "Date" },
    field_name{ field::host, "Host" },
    field_name{ field::keep_alive, "Keep-Alive" },
    field_name{ field::location, "Location" },
    field_name{ field::origin, "Origin" },
    field_name{ field::sec_websocket_accept, "Sec-WebSocket-Accept" },
    field_name{ field::sec_websocket_extensions, "Sec-WebSocket-Extensions" },
    field_name{ field::sec_websocket_key, "Sec-WebSocket-Key" },
    field_name{ field::sec_websocket_protocol, "Sec-WebSocket-Protocol" },
    field_name{ field::sec_websocket_version, "Sec-WebSocket-Version" },
    field_name{ field::server, "Server" },
    field_name{ field::transfer_encoding, "Transfer-Encoding" },
    field_name{ field::upgrade, "Upgrade" },
    field_name{ field::user_agent, "User-Agent" },
    field_name{ field::vary, "Vary" }
};

field to_field(const std::string_view& name) NOEXCEPT
{
    const auto it = std::ranges::find_if(field_names, [&name](const auto& item)
        NOEXCEPT { return iequals(item.text, name); });

    return it == field_names.end() ? field::unknown : it->name;
}

std::string_view to_string(field name) NOEXCEPT
{
    const auto it = std::ranges::find(field_names, name, &field_name::name);
    return it == field_names.end() ? std::string_view{} : it->text;
}

} // namespace http
} // namespace network
} // namespace libbitcoin
