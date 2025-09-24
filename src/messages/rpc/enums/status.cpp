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
#include <bitcoin/network/messages/rpc/enums/status.hpp>

#include <unordered_map>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// datatracker.ietf.org/doc/html/rfc9110#name-status-codes

status to_status(const std::string& value) NOEXCEPT
{
    static const std::unordered_map<std::string, status> map
    {
        /// 1xx Informational
        { "100 CONTINUE", status::continue_ },
        { "101 SWITCHING_PROTOCOLS", status::switching_protocols },
        { "102 PROCESSING", status::processing },
        { "103 EARLY_HINTS", status::early_hints },

        /// 2xx Success
        { "200 OK", status::ok },
        { "201 CREATED", status::created },
        { "202 ACCEPTED", status::accepted },
        { "203 NON_AUTHORITATIVE_INFORMATION", status::non_authoritative_information },
        { "204 NO_CONTENT", status::no_content },
        { "205 RESET_CONTENT", status::reset_content },
        { "206 PARTIAL_CONTENT", status::partial_content },
        { "207 MULTI_STATUS", status::multi_status },
        { "208 ALREADY_REPORTED", status::already_reported },
        { "226 IM_USED", status::im_used },

        /// 3xx Redirection
        { "300 MULTIPLE_CHOICES", status::multiple_choices },
        { "301 MOVED_PERMANENTLY", status::moved_permanently },
        { "302 FOUND", status::found },
        { "303 SEE_OTHER", status::see_other },
        { "304 NOT_MODIFIED", status::not_modified },
        { "305 USE_PROXY", status::use_proxy },
        { "307 TEMPORARY_REDIRECT", status::temporary_redirect },
        { "308 PERMANENT_REDIRECT", status::permanent_redirect },

        /// 4xx Client Error
        { "400 BAD_REQUEST", status::bad_request },
        { "401 UNAUTHORIZED", status::unauthorized },
        { "402 PAYMENT_REQUIRED", status::payment_required },
        { "403 FORBIDDEN", status::forbidden },
        { "404 NOT_FOUND", status::not_found },
        { "405 METHOD_NOT_ALLOWED", status::method_not_allowed },
        { "406 NOT_ACCEPTABLE", status::not_acceptable },
        { "407 PROXY_AUTHENTICATION_REQUIRED", status::proxy_authentication_required },
        { "408 REQUEST_TIMEOUT", status::request_timeout },
        { "409 CONFLICT", status::conflict },
        { "410 GONE", status::gone },
        { "411 LENGTH_REQUIRED", status::length_required },
        { "412 PRECONDITION_FAILED", status::precondition_failed },
        { "413 PAYLOAD_TOO_LARGE", status::payload_too_large },
        { "414 URI_TOO_LONG", status::uri_too_long },
        { "415 UNSUPPORTED_MEDIA_TYPE", status::unsupported_media_type },
        { "416 RANGE_NOT_SATISFIABLE", status::range_not_satisfiable },
        { "417 EXPECTATION_FAILED", status::expectation_failed },
        { "418 IM_A_TEAPOT", status::im_a_teapot },
        { "421 MISDIRECTED_REQUEST", status::misdirected_request },
        { "422 UNPROCESSABLE_ENTITY", status::unprocessable_entity },
        { "423 LOCKED", status::locked },
        { "424 FAILED_DEPENDENCY", status::failed_dependency },
        { "425 TOO_EARLY", status::too_early },
        { "426 UPGRADE_REQUIRED", status::upgrade_required },
        { "428 PRECONDITION_REQUIRED", status::precondition_required },
        { "429 TOO_MANY_REQUESTS", status::too_many_requests },
        { "431 REQUEST_HEADER_FIELDS_TOO_LARGE", status::request_header_fields_too_large },
        { "451 UNAVAILABLE_FOR_LEGAL_REASONS", status::unavailable_for_legal_reasons },

        /// 5xx Server Error
        { "500 INTERNAL_SERVER_ERROR", status::internal_server_error },
        { "501 NOT_IMPLEMENTED", status::not_implemented },
        { "502 BAD_GATEWAY", status::bad_gateway },
        { "503 SERVICE_UNAVAILABLE", status::service_unavailable },
        { "504 GATEWAY_TIMEOUT", status::gateway_timeout },
        { "505 HTTP_VERSION_NOT_SUPPORTED", status::http_version_not_supported },
        { "506 VARIANT_ALSO_NEGOTIATES", status::variant_also_negotiates },
        { "507 INSUFFICIENT_STORAGE", status::insufficient_storage },
        { "508 LOOP_DETECTED", status::loop_detected },
        { "510 NOT_EXTENDED", status::not_extended },
        { "511 NETWORK_AUTHENTICATION_REQUIRED", status::network_authentication_required },

        /// Default
        { "000 UNDEFINED", status::undefined }
    };

    const auto found = map.find(value);
    return found == map.end() ? status::undefined : found->second;
}

const std::string& from_status(status value) NOEXCEPT
{
    static const std::unordered_map<status, std::string> map
    {
        // 1xx Informational
        { status::continue_, "100 CONTINUE" },
        { status::switching_protocols, "101 SWITCHING_PROTOCOLS" },
        { status::processing, "102 PROCESSING" },
        { status::early_hints, "103 EARLY_HINTS" },

        // 2xx Success
        { status::ok, "200 OK" },
        { status::created, "201 CREATED" },
        { status::accepted, "202 ACCEPTED" },
        { status::non_authoritative_information, "203 NON_AUTHORITATIVE_INFORMATION" },
        { status::no_content, "204 NO_CONTENT" },
        { status::reset_content, "205 RESET_CONTENT" },
        { status::partial_content, "206 PARTIAL_CONTENT" },
        { status::multi_status, "207 MULTI_STATUS" },
        { status::already_reported, "208 ALREADY_REPORTED" },
        { status::im_used, "226 IM_USED" },

        // 3xx Redirection
        { status::multiple_choices, "300 MULTIPLE_CHOICES" },
        { status::moved_permanently, "301 MOVED_PERMANENTLY" },
        { status::found, "302 FOUND" },
        { status::see_other, "303 SEE_OTHER" },
        { status::not_modified, "304 NOT_MODIFIED" },
        { status::use_proxy, "305 USE_PROXY" },
        { status::temporary_redirect, "307 TEMPORARY_REDIRECT" },
        { status::permanent_redirect, "308 PERMANENT_REDIRECT" },

        // 4xx Client Error
        { status::bad_request, "400 BAD_REQUEST" },
        { status::unauthorized, "401 UNAUTHORIZED" },
        { status::payment_required, "402 PAYMENT_REQUIRED" },
        { status::forbidden, "403 FORBIDDEN" },
        { status::not_found, "404 NOT_FOUND" },
        { status::method_not_allowed, "405 METHOD_NOT_ALLOWED" },
        { status::not_acceptable, "406 NOT_ACCEPTABLE" },
        { status::proxy_authentication_required, "407 PROXY_AUTHENTICATION_REQUIRED" },
        { status::request_timeout, "408 REQUEST_TIMEOUT" },
        { status::conflict, "409 CONFLICT" },
        { status::gone, "410 GONE" },
        { status::length_required, "411 LENGTH_REQUIRED" },
        { status::precondition_failed, "412 PRECONDITION_FAILED" },
        { status::payload_too_large, "413 PAYLOAD_TOO_LARGE" },
        { status::uri_too_long, "414 URI_TOO_LONG" },
        { status::unsupported_media_type, "415 UNSUPPORTED_MEDIA_TYPE" },
        { status::range_not_satisfiable, "416 RANGE_NOT_SATISFIABLE" },
        { status::expectation_failed, "417 EXPECTATION_FAILED" },
        { status::im_a_teapot, "418 IM_A_TEAPOT" },
        { status::misdirected_request, "421 MISDIRECTED_REQUEST" },
        { status::unprocessable_entity, "422 UNPROCESSABLE_ENTITY" },
        { status::locked, "423 LOCKED" },
        { status::failed_dependency, "424 FAILED_DEPENDENCY" },
        { status::too_early, "425 TOO_EARLY" },
        { status::upgrade_required, "426 UPGRADE_REQUIRED" },
        { status::precondition_required, "428 PRECONDITION_REQUIRED" },
        { status::too_many_requests, "429 TOO_MANY_REQUESTS" },
        { status::request_header_fields_too_large, "431 REQUEST_HEADER_FIELDS_TOO_LARGE" },
        { status::unavailable_for_legal_reasons, "451 UNAVAILABLE_FOR_LEGAL_REASONS" },

        // 5xx Server Error
        { status::internal_server_error, "500 INTERNAL_SERVER_ERROR" },
        { status::not_implemented, "501 NOT_IMPLEMENTED" },
        { status::bad_gateway, "502 BAD_GATEWAY" },
        { status::service_unavailable, "503 SERVICE_UNAVAILABLE" },
        { status::gateway_timeout, "504 GATEWAY_TIMEOUT" },
        { status::http_version_not_supported, "505 HTTP_VERSION_NOT_SUPPORTED" },
        { status::variant_also_negotiates, "506 VARIANT_ALSO_NEGOTIATES" },
        { status::insufficient_storage, "507 INSUFFICIENT_STORAGE" },
        { status::loop_detected, "508 LOOP_DETECTED" },
        { status::not_extended, "510 NOT_EXTENDED" },
        { status::network_authentication_required, "511 NETWORK_AUTHENTICATION_REQUIRED" },

        // Default
        { status::undefined, "000 UNDEFINED" }
    };

    return map.at(value);
}

BC_POP_WARNING()

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin
