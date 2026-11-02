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
#include <bitcoin/network/messages/http/enums/status.hpp>

#include <algorithm>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

struct status_name
{
    status code;
    std::string_view reason;
};

static constexpr std::array<status_name, 64> statuses
{
    status_name{ status::continue_, "Continue" },
    status_name{ status::switching_protocols, "Switching Protocols" },
    status_name{ status::processing, "Processing" },
    status_name{ status::early_hints, "Early Hints" },
    status_name{ status::ok, "OK" },
    status_name{ status::created, "Created" },
    status_name{ status::accepted, "Accepted" },
    status_name{ status::non_authoritative_information, "Non-Authoritative Information" },
    status_name{ status::no_content, "No Content" },
    status_name{ status::reset_content, "Reset Content" },
    status_name{ status::partial_content, "Partial Content" },
    status_name{ status::multi_status, "Multi-Status" },
    status_name{ status::already_reported, "Already Reported" },
    status_name{ status::im_used, "IM Used" },
    status_name{ status::multiple_choices, "Multiple Choices" },
    status_name{ status::moved_permanently, "Moved Permanently" },
    status_name{ status::found, "Found" },
    status_name{ status::see_other, "See Other" },
    status_name{ status::not_modified, "Not Modified" },
    status_name{ status::use_proxy, "Use Proxy" },
    status_name{ status::temporary_redirect, "Temporary Redirect" },
    status_name{ status::permanent_redirect, "Permanent Redirect" },
    status_name{ status::bad_request, "Bad Request" },
    status_name{ status::unauthorized, "Unauthorized" },
    status_name{ status::payment_required, "Payment Required" },
    status_name{ status::forbidden, "Forbidden" },
    status_name{ status::not_found, "Not Found" },
    status_name{ status::method_not_allowed, "Method Not Allowed" },
    status_name{ status::not_acceptable, "Not Acceptable" },
    status_name{ status::proxy_authentication_required, "Proxy Authentication Required" },
    status_name{ status::request_timeout, "Request Timeout" },
    status_name{ status::conflict, "Conflict" },
    status_name{ status::gone, "Gone" },
    status_name{ status::length_required, "Length Required" },
    status_name{ status::precondition_failed, "Precondition Failed" },
    status_name{ status::payload_too_large, "Payload Too Large" },
    status_name{ status::uri_too_long, "URI Too Long" },
    status_name{ status::unsupported_media_type, "Unsupported Media Type" },
    status_name{ status::range_not_satisfiable, "Range Not Satisfiable" },
    status_name{ status::expectation_failed, "Expectation Failed" },
    status_name{ status::misdirected_request, "Misdirected Request" },
    status_name{ status::unprocessable_entity, "Unprocessable Entity" },
    status_name{ status::locked, "Locked" },
    status_name{ status::failed_dependency, "Failed Dependency" },
    status_name{ status::too_early, "Too Early" },
    status_name{ status::upgrade_required, "Upgrade Required" },
    status_name{ status::precondition_required, "Precondition Required" },
    status_name{ status::too_many_requests, "Too Many Requests" },
    status_name{ status::request_header_fields_too_large, "Request Header Fields Too Large" },
    status_name{ status::connection_closed_without_response, "Connection Closed Without Response" },
    status_name{ status::unavailable_for_legal_reasons, "Unavailable For Legal Reasons" },
    status_name{ status::client_closed_request, "Client Closed Request" },
    status_name{ status::internal_server_error, "Internal Server Error" },
    status_name{ status::not_implemented, "Not Implemented" },
    status_name{ status::bad_gateway, "Bad Gateway" },
    status_name{ status::service_unavailable, "Service Unavailable" },
    status_name{ status::gateway_timeout, "Gateway Timeout" },
    status_name{ status::http_version_not_supported, "HTTP Version Not Supported" },
    status_name{ status::variant_also_negotiates, "Variant Also Negotiates" },
    status_name{ status::insufficient_storage, "Insufficient Storage" },
    status_name{ status::loop_detected, "Loop Detected" },
    status_name{ status::not_extended, "Not Extended" },
    status_name{ status::network_authentication_required, "Network Authentication Required" },
    status_name{ status::network_connect_timeout_error, "Network Connect Timeout Error" }
};

status to_status(uint32_t code) NOEXCEPT
{
    const auto it = std::ranges::find_if(statuses, [code](const auto& item)
        NOEXCEPT { return to_value(item.code) == code; });

    return it == statuses.end() ? status::unknown : it->code;
}

std::string status_string(status code) NOEXCEPT
{
    const auto it = std::ranges::find(statuses, code, &status_name::code);
    return std::string{ it == statuses.end() ? "<unknown-status>" :
        it->reason };
}

} // namespace http
} // namespace network
} // namespace libbitcoin
