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

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace rpc {

enum class status
{
    /// 000 Default
    undefined,

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
    network_authentication_required
};

BCT_API status to_status(const std::string& value) NOEXCEPT;
BCT_API const std::string& from_status(status value) NOEXCEPT;

} // namespace rpc
} // namespace messages
} // namespace network
} // namespace libbitcoin

#endif
