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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_STATUS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_STATUS_HPP

#include <unordered_map>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

enum class status
{
    /// 1xx Informational
    continue_,
    switching_protocols,
    processing,
    early_hints,

    /// 2xx Success
    ok,
    created,
    accepted,
    non_authoritative_information,
    no_content,
    reset_content,
    partial_content,
    multi_status,
    already_reported,
    im_used,

    /// 3xx Redirection
    multiple_choices,
    moved_permanently,
    found,
    see_other,
    not_modified,
    use_proxy,
    temporary_redirect,
    permanent_redirect,

    /// 4xx Client Error
    bad_request,
    unauthorized,
    payment_required,
    forbidden,
    not_found,
    method_not_allowed,
    not_acceptable,
    proxy_authentication_required,
    request_timeout,
    conflict,
    gone,
    length_required,
    precondition_failed,
    payload_too_large,
    uri_too_long,
    unsupported_media_type,
    range_not_satisfiable,
    expectation_failed,
    im_a_teapot,
    misdirected_request,
    unprocessable_entity,
    locked,
    failed_dependency,
    too_early,
    upgrade_required,
    precondition_required,
    too_many_requests,
    request_header_fields_too_large,
    unavailable_for_legal_reasons,

    /// 5xx Server Error
    internal_server_error,
    not_implemented,
    bad_gateway,
    service_unavailable,
    gateway_timeout,
    http_version_not_supported,
    variant_also_negotiates,
    insufficient_storage,
    loop_detected,
    not_extended,
    network_authentication_required,

    /// Default
    undefined
};

inline status to_status(const std::string& value) NOEXCEPT
{
    static const std::unordered_map<std::string, status> map
    {
        /// 1xx Informational
        { "CONTINUE", status::continue_ },
        { "SWITCHING_PROTOCOLS", status::switching_protocols },
        { "PROCESSING", status::processing },
        { "EARLY_HINTS", status::early_hints },

        /// 2xx Success
        { "OK", status::ok },
        { "CREATED", status::created },
        { "ACCEPTED", status::accepted },
        { "NON_AUTHORITATIVE_INFORMATION", status::non_authoritative_information },
        { "NO_CONTENT", status::no_content },
        { "RESET_CONTENT", status::reset_content },
        { "PARTIAL_CONTENT", status::partial_content },
        { "MULTI_STATUS", status::multi_status },
        { "ALREADY_REPORTED", status::already_reported },
        { "IM_USED", status::im_used },

        /// 3xx Redirection
        { "MULTIPLE_CHOICES", status::multiple_choices },
        { "MOVED_PERMANENTLY", status::moved_permanently },
        { "FOUND", status::found },
        { "SEE_OTHER", status::see_other },
        { "NOT_MODIFIED", status::not_modified },
        { "USE_PROXY", status::use_proxy },
        { "TEMPORARY_REDIRECT", status::temporary_redirect },
        { "PERMANENT_REDIRECT", status::permanent_redirect },

        /// 4xx Client Error
        { "BAD_REQUEST", status::bad_request },
        { "UNAUTHORIZED", status::unauthorized },
        { "PAYMENT_REQUIRED", status::payment_required },
        { "FORBIDDEN", status::forbidden },
        { "NOT_FOUND", status::not_found },
        { "METHOD_NOT_ALLOWED", status::method_not_allowed },
        { "NOT_ACCEPTABLE", status::not_acceptable },
        { "PROXY_AUTHENTICATION_REQUIRED", status::proxy_authentication_required },
        { "REQUEST_TIMEOUT", status::request_timeout },
        { "CONFLICT", status::conflict },
        { "GONE", status::gone },
        { "LENGTH_REQUIRED", status::length_required },
        { "PRECONDITION_FAILED", status::precondition_failed },
        { "PAYLOAD_TOO_LARGE", status::payload_too_large },
        { "URI_TOO_LONG", status::uri_too_long },
        { "UNSUPPORTED_MEDIA_TYPE", status::unsupported_media_type },
        { "RANGE_NOT_SATISFIABLE", status::range_not_satisfiable },
        { "EXPECTATION_FAILED", status::expectation_failed },
        { "IM_A_TEAPOT", status::im_a_teapot },
        { "MISDIRECTED_REQUEST", status::misdirected_request },
        { "UNPROCESSABLE_ENTITY", status::unprocessable_entity },
        { "LOCKED", status::locked },
        { "FAILED_DEPENDENCY", status::failed_dependency },
        { "TOO_EARLY", status::too_early },
        { "UPGRADE_REQUIRED", status::upgrade_required },
        { "PRECONDITION_REQUIRED", status::precondition_required },
        { "TOO_MANY_REQUESTS", status::too_many_requests },
        { "REQUEST_HEADER_FIELDS_TOO_LARGE", status::request_header_fields_too_large },
        { "UNAVAILABLE_FOR_LEGAL_REASONS", status::unavailable_for_legal_reasons },

        /// 5xx Server Error
        { "INTERNAL_SERVER_ERROR", status::internal_server_error },
        { "NOT_IMPLEMENTED", status::not_implemented },
        { "BAD_GATEWAY", status::bad_gateway },
        { "SERVICE_UNAVAILABLE", status::service_unavailable },
        { "GATEWAY_TIMEOUT", status::gateway_timeout },
        { "HTTP_VERSION_NOT_SUPPORTED", status::http_version_not_supported },
        { "VARIANT_ALSO_NEGOTIATES", status::variant_also_negotiates },
        { "INSUFFICIENT_STORAGE", status::insufficient_storage },
        { "LOOP_DETECTED", status::loop_detected },
        { "NOT_EXTENDED", status::not_extended },
        { "NETWORK_AUTHENTICATION_REQUIRED", status::network_authentication_required },

        /// Default
        { "undefined", status::undefined }
    };

    const auto found = map.find(value);
    return found == map.end() ? status::undefined : found->second;
}

inline std::string from_status(status value) NOEXCEPT
{
    static const std::unordered_map<status, std::string> map
    {
        // 1xx Informational
        { status::continue_, "CONTINUE" },
        { status::switching_protocols, "SWITCHING_PROTOCOLS" },
        { status::processing, "PROCESSING" },
        { status::early_hints, "EARLY_HINTS" },

        // 2xx Success
        { status::ok, "OK" },
        { status::created, "CREATED" },
        { status::accepted, "ACCEPTED" },
        { status::non_authoritative_information, "NON_AUTHORITATIVE_INFORMATION" },
        { status::no_content, "NO_CONTENT" },
        { status::reset_content, "RESET_CONTENT" },
        { status::partial_content, "PARTIAL_CONTENT" },
        { status::multi_status, "MULTI_STATUS" },
        { status::already_reported, "ALREADY_REPORTED" },
        { status::im_used, "IM_USED" },

        // 3xx Redirection
        { status::multiple_choices, "MULTIPLE_CHOICES" },
        { status::moved_permanently, "MOVED_PERMANENTLY" },
        { status::found, "FOUND" },
        { status::see_other, "SEE_OTHER" },
        { status::not_modified, "NOT_MODIFIED" },
        { status::use_proxy, "USE_PROXY" },
        { status::temporary_redirect, "TEMPORARY_REDIRECT" },
        { status::permanent_redirect, "PERMANENT_REDIRECT" },

        // 4xx Client Error
        { status::bad_request, "BAD_REQUEST" },
        { status::unauthorized, "UNAUTHORIZED" },
        { status::payment_required, "PAYMENT_REQUIRED" },
        { status::forbidden, "FORBIDDEN" },
        { status::not_found, "NOT_FOUND" },
        { status::method_not_allowed, "METHOD_NOT_ALLOWED" },
        { status::not_acceptable, "NOT_ACCEPTABLE" },
        { status::proxy_authentication_required, "PROXY_AUTHENTICATION_REQUIRED" },
        { status::request_timeout, "REQUEST_TIMEOUT" },
        { status::conflict, "CONFLICT" },
        { status::gone, "GONE" },
        { status::length_required, "LENGTH_REQUIRED" },
        { status::precondition_failed, "PRECONDITION_FAILED" },
        { status::payload_too_large, "PAYLOAD_TOO_LARGE" },
        { status::uri_too_long, "URI_TOO_LONG" },
        { status::unsupported_media_type, "UNSUPPORTED_MEDIA_TYPE" },
        { status::range_not_satisfiable, "RANGE_NOT_SATISFIABLE" },
        { status::expectation_failed, "EXPECTATION_FAILED" },
        { status::im_a_teapot, "IM_A_TEAPOT" },
        { status::misdirected_request, "MISDIRECTED_REQUEST" },
        { status::unprocessable_entity, "UNPROCESSABLE_ENTITY" },
        { status::locked, "LOCKED" },
        { status::failed_dependency, "FAILED_DEPENDENCY" },
        { status::too_early, "TOO_EARLY" },
        { status::upgrade_required, "UPGRADE_REQUIRED" },
        { status::precondition_required, "PRECONDITION_REQUIRED" },
        { status::too_many_requests, "TOO_MANY_REQUESTS" },
        { status::request_header_fields_too_large, "REQUEST_HEADER_FIELDS_TOO_LARGE" },
        { status::unavailable_for_legal_reasons, "UNAVAILABLE_FOR_LEGAL_REASONS" },

        // 5xx Server Error
        { status::internal_server_error, "INTERNAL_SERVER_ERROR" },
        { status::not_implemented, "NOT_IMPLEMENTED" },
        { status::bad_gateway, "BAD_GATEWAY" },
        { status::service_unavailable, "SERVICE_UNAVAILABLE" },
        { status::gateway_timeout, "GATEWAY_TIMEOUT" },
        { status::http_version_not_supported, "HTTP_VERSION_NOT_SUPPORTED" },
        { status::variant_also_negotiates, "VARIANT_ALSO_NEGOTIATES" },
        { status::insufficient_storage, "INSUFFICIENT_STORAGE" },
        { status::loop_detected, "LOOP_DETECTED" },
        { status::not_extended, "NOT_EXTENDED" },
        { status::network_authentication_required, "NETWORK_AUTHENTICATION_REQUIRED" },

        // Default
        { status::undefined, "undefined" }
    };

    return map.at(value);
}

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
