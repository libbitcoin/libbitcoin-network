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
#include "../../../test.hpp"

BOOST_AUTO_TEST_SUITE(rpc_status_tests)

using namespace network::messages::rpc;

BOOST_AUTO_TEST_CASE(rpc_status__to_status__always__expected)
{
    // 000 Default
    BOOST_REQUIRE(to_status("000 UNDEFINED") == status::undefined);

    // 1xx Informational
    BOOST_REQUIRE(to_status("100 CONTINUE") == status::continue_);
    BOOST_REQUIRE(to_status("101 SWITCHING_PROTOCOLS") == status::switching_protocols);
    BOOST_REQUIRE(to_status("102 PROCESSING") == status::processing);
    BOOST_REQUIRE(to_status("103 EARLY_HINTS") == status::early_hints);

    // 2xx Success
    BOOST_REQUIRE(to_status("200 OK") == status::ok);
    BOOST_REQUIRE(to_status("201 CREATED") == status::created);
    BOOST_REQUIRE(to_status("202 ACCEPTED") == status::accepted);
    BOOST_REQUIRE(to_status("203 NON_AUTHORITATIVE_INFORMATION") == status::non_authoritative_information);
    BOOST_REQUIRE(to_status("204 NO_CONTENT") == status::no_content);
    BOOST_REQUIRE(to_status("205 RESET_CONTENT") == status::reset_content);
    BOOST_REQUIRE(to_status("206 PARTIAL_CONTENT") == status::partial_content);
    BOOST_REQUIRE(to_status("207 MULTI_STATUS") == status::multi_status);
    BOOST_REQUIRE(to_status("208 ALREADY_REPORTED") == status::already_reported);
    BOOST_REQUIRE(to_status("226 IM_USED") == status::im_used);

    // 3xx Redirection
    BOOST_REQUIRE(to_status("300 MULTIPLE_CHOICES") == status::multiple_choices);
    BOOST_REQUIRE(to_status("301 MOVED_PERMANENTLY") == status::moved_permanently);
    BOOST_REQUIRE(to_status("302 FOUND") == status::found);
    BOOST_REQUIRE(to_status("303 SEE_OTHER") == status::see_other);
    BOOST_REQUIRE(to_status("304 NOT_MODIFIED") == status::not_modified);
    BOOST_REQUIRE(to_status("305 USE_PROXY") == status::use_proxy);
    BOOST_REQUIRE(to_status("307 TEMPORARY_REDIRECT") == status::temporary_redirect);
    BOOST_REQUIRE(to_status("308 PERMANENT_REDIRECT") == status::permanent_redirect);

    // 4xx Client Error
    BOOST_REQUIRE(to_status("400 BAD_REQUEST") == status::bad_request);
    BOOST_REQUIRE(to_status("401 UNAUTHORIZED") == status::unauthorized);
    BOOST_REQUIRE(to_status("402 PAYMENT_REQUIRED") == status::payment_required);
    BOOST_REQUIRE(to_status("403 FORBIDDEN") == status::forbidden);
    BOOST_REQUIRE(to_status("404 NOT_FOUND") == status::not_found);
    BOOST_REQUIRE(to_status("405 METHOD_NOT_ALLOWED") == status::method_not_allowed);
    BOOST_REQUIRE(to_status("406 NOT_ACCEPTABLE") == status::not_acceptable);
    BOOST_REQUIRE(to_status("407 PROXY_AUTHENTICATION_REQUIRED") == status::proxy_authentication_required);
    BOOST_REQUIRE(to_status("408 REQUEST_TIMEOUT") == status::request_timeout);
    BOOST_REQUIRE(to_status("409 CONFLICT") == status::conflict);
    BOOST_REQUIRE(to_status("410 GONE") == status::gone);
    BOOST_REQUIRE(to_status("411 LENGTH_REQUIRED") == status::length_required);
    BOOST_REQUIRE(to_status("412 PRECONDITION_FAILED") == status::precondition_failed);
    BOOST_REQUIRE(to_status("413 PAYLOAD_TOO_LARGE") == status::payload_too_large);
    BOOST_REQUIRE(to_status("414 URI_TOO_LONG") == status::uri_too_long);
    BOOST_REQUIRE(to_status("415 UNSUPPORTED_MEDIA_TYPE") == status::unsupported_media_type);
    BOOST_REQUIRE(to_status("416 RANGE_NOT_SATISFIABLE") == status::range_not_satisfiable);
    BOOST_REQUIRE(to_status("417 EXPECTATION_FAILED") == status::expectation_failed);
    BOOST_REQUIRE(to_status("418 IM_A_TEAPOT") == status::im_a_teapot);
    BOOST_REQUIRE(to_status("421 MISDIRECTED_REQUEST") == status::misdirected_request);
    BOOST_REQUIRE(to_status("422 UNPROCESSABLE_ENTITY") == status::unprocessable_entity);
    BOOST_REQUIRE(to_status("423 LOCKED") == status::locked);
    BOOST_REQUIRE(to_status("424 FAILED_DEPENDENCY") == status::failed_dependency);
    BOOST_REQUIRE(to_status("425 TOO_EARLY") == status::too_early);
    BOOST_REQUIRE(to_status("426 UPGRADE_REQUIRED") == status::upgrade_required);
    BOOST_REQUIRE(to_status("428 PRECONDITION_REQUIRED") == status::precondition_required);
    BOOST_REQUIRE(to_status("429 TOO_MANY_REQUESTS") == status::too_many_requests);
    BOOST_REQUIRE(to_status("431 REQUEST_HEADER_FIELDS_TOO_LARGE") == status::request_header_fields_too_large);
    BOOST_REQUIRE(to_status("451 UNAVAILABLE_FOR_LEGAL_REASONS") == status::unavailable_for_legal_reasons);

    // 5xx Server Error
    BOOST_REQUIRE(to_status("500 INTERNAL_SERVER_ERROR") == status::internal_server_error);
    BOOST_REQUIRE(to_status("501 NOT_IMPLEMENTED") == status::not_implemented);
    BOOST_REQUIRE(to_status("502 BAD_GATEWAY") == status::bad_gateway);
    BOOST_REQUIRE(to_status("503 SERVICE_UNAVAILABLE") == status::service_unavailable);
    BOOST_REQUIRE(to_status("504 GATEWAY_TIMEOUT") == status::gateway_timeout);
    BOOST_REQUIRE(to_status("505 HTTP_VERSION_NOT_SUPPORTED") == status::http_version_not_supported);
    BOOST_REQUIRE(to_status("506 VARIANT_ALSO_NEGOTIATES") == status::variant_also_negotiates);
    BOOST_REQUIRE(to_status("507 INSUFFICIENT_STORAGE") == status::insufficient_storage);
    BOOST_REQUIRE(to_status("508 LOOP_DETECTED") == status::loop_detected);
    BOOST_REQUIRE(to_status("510 NOT_EXTENDED") == status::not_extended);
    BOOST_REQUIRE(to_status("511 NETWORK_AUTHENTICATION_REQUIRED") == status::network_authentication_required);
}

BOOST_AUTO_TEST_CASE(rpc_status__from_status__always__expected)
{
    // 000 Default
    BOOST_REQUIRE_EQUAL(from_status(status::undefined), "000 UNDEFINED");

    // 1xx Informational
    BOOST_REQUIRE_EQUAL(from_status(status::continue_), "100 CONTINUE");
    BOOST_REQUIRE_EQUAL(from_status(status::switching_protocols), "101 SWITCHING_PROTOCOLS");
    BOOST_REQUIRE_EQUAL(from_status(status::processing), "102 PROCESSING");
    BOOST_REQUIRE_EQUAL(from_status(status::early_hints), "103 EARLY_HINTS");

    // 2xx Success
    BOOST_REQUIRE_EQUAL(from_status(status::ok), "200 OK");
    BOOST_REQUIRE_EQUAL(from_status(status::created), "201 CREATED");
    BOOST_REQUIRE_EQUAL(from_status(status::accepted), "202 ACCEPTED");
    BOOST_REQUIRE_EQUAL(from_status(status::non_authoritative_information), "203 NON_AUTHORITATIVE_INFORMATION");
    BOOST_REQUIRE_EQUAL(from_status(status::no_content), "204 NO_CONTENT");
    BOOST_REQUIRE_EQUAL(from_status(status::reset_content), "205 RESET_CONTENT");
    BOOST_REQUIRE_EQUAL(from_status(status::partial_content), "206 PARTIAL_CONTENT");
    BOOST_REQUIRE_EQUAL(from_status(status::multi_status), "207 MULTI_STATUS");
    BOOST_REQUIRE_EQUAL(from_status(status::already_reported), "208 ALREADY_REPORTED");
    BOOST_REQUIRE_EQUAL(from_status(status::im_used), "226 IM_USED");

    // 3xx Redirection
    BOOST_REQUIRE_EQUAL(from_status(status::multiple_choices), "300 MULTIPLE_CHOICES");
    BOOST_REQUIRE_EQUAL(from_status(status::moved_permanently), "301 MOVED_PERMANENTLY");
    BOOST_REQUIRE_EQUAL(from_status(status::found), "302 FOUND");
    BOOST_REQUIRE_EQUAL(from_status(status::see_other), "303 SEE_OTHER");
    BOOST_REQUIRE_EQUAL(from_status(status::not_modified), "304 NOT_MODIFIED");
    BOOST_REQUIRE_EQUAL(from_status(status::use_proxy), "305 USE_PROXY");
    BOOST_REQUIRE_EQUAL(from_status(status::temporary_redirect), "307 TEMPORARY_REDIRECT");
    BOOST_REQUIRE_EQUAL(from_status(status::permanent_redirect), "308 PERMANENT_REDIRECT");

    // 4xx Client Error
    BOOST_REQUIRE_EQUAL(from_status(status::bad_request), "400 BAD_REQUEST");
    BOOST_REQUIRE_EQUAL(from_status(status::unauthorized), "401 UNAUTHORIZED");
    BOOST_REQUIRE_EQUAL(from_status(status::payment_required), "402 PAYMENT_REQUIRED");
    BOOST_REQUIRE_EQUAL(from_status(status::forbidden), "403 FORBIDDEN");
    BOOST_REQUIRE_EQUAL(from_status(status::not_found), "404 NOT_FOUND");
    BOOST_REQUIRE_EQUAL(from_status(status::method_not_allowed), "405 METHOD_NOT_ALLOWED");
    BOOST_REQUIRE_EQUAL(from_status(status::not_acceptable), "406 NOT_ACCEPTABLE");
    BOOST_REQUIRE_EQUAL(from_status(status::proxy_authentication_required), "407 PROXY_AUTHENTICATION_REQUIRED");
    BOOST_REQUIRE_EQUAL(from_status(status::request_timeout), "408 REQUEST_TIMEOUT");
    BOOST_REQUIRE_EQUAL(from_status(status::conflict), "409 CONFLICT");
    BOOST_REQUIRE_EQUAL(from_status(status::gone), "410 GONE");
    BOOST_REQUIRE_EQUAL(from_status(status::length_required), "411 LENGTH_REQUIRED");
    BOOST_REQUIRE_EQUAL(from_status(status::precondition_failed), "412 PRECONDITION_FAILED");
    BOOST_REQUIRE_EQUAL(from_status(status::payload_too_large), "413 PAYLOAD_TOO_LARGE");
    BOOST_REQUIRE_EQUAL(from_status(status::uri_too_long), "414 URI_TOO_LONG");
    BOOST_REQUIRE_EQUAL(from_status(status::unsupported_media_type), "415 UNSUPPORTED_MEDIA_TYPE");
    BOOST_REQUIRE_EQUAL(from_status(status::range_not_satisfiable), "416 RANGE_NOT_SATISFIABLE");
    BOOST_REQUIRE_EQUAL(from_status(status::expectation_failed), "417 EXPECTATION_FAILED");
    BOOST_REQUIRE_EQUAL(from_status(status::im_a_teapot), "418 IM_A_TEAPOT");
    BOOST_REQUIRE_EQUAL(from_status(status::misdirected_request), "421 MISDIRECTED_REQUEST");
    BOOST_REQUIRE_EQUAL(from_status(status::unprocessable_entity), "422 UNPROCESSABLE_ENTITY");
    BOOST_REQUIRE_EQUAL(from_status(status::locked), "423 LOCKED");
    BOOST_REQUIRE_EQUAL(from_status(status::failed_dependency), "424 FAILED_DEPENDENCY");
    BOOST_REQUIRE_EQUAL(from_status(status::too_early), "425 TOO_EARLY");
    BOOST_REQUIRE_EQUAL(from_status(status::upgrade_required), "426 UPGRADE_REQUIRED");
    BOOST_REQUIRE_EQUAL(from_status(status::precondition_required), "428 PRECONDITION_REQUIRED");
    BOOST_REQUIRE_EQUAL(from_status(status::too_many_requests), "429 TOO_MANY_REQUESTS");
    BOOST_REQUIRE_EQUAL(from_status(status::request_header_fields_too_large), "431 REQUEST_HEADER_FIELDS_TOO_LARGE");
    BOOST_REQUIRE_EQUAL(from_status(status::unavailable_for_legal_reasons), "451 UNAVAILABLE_FOR_LEGAL_REASONS");

    // 5xx Server Error
    BOOST_REQUIRE_EQUAL(from_status(status::internal_server_error), "500 INTERNAL_SERVER_ERROR");
    BOOST_REQUIRE_EQUAL(from_status(status::not_implemented), "501 NOT_IMPLEMENTED");
    BOOST_REQUIRE_EQUAL(from_status(status::bad_gateway), "502 BAD_GATEWAY");
    BOOST_REQUIRE_EQUAL(from_status(status::service_unavailable), "503 SERVICE_UNAVAILABLE");
    BOOST_REQUIRE_EQUAL(from_status(status::gateway_timeout), "504 GATEWAY_TIMEOUT");
    BOOST_REQUIRE_EQUAL(from_status(status::http_version_not_supported), "505 HTTP_VERSION_NOT_SUPPORTED");
    BOOST_REQUIRE_EQUAL(from_status(status::variant_also_negotiates), "506 VARIANT_ALSO_NEGOTIATES");
    BOOST_REQUIRE_EQUAL(from_status(status::insufficient_storage), "507 INSUFFICIENT_STORAGE");
    BOOST_REQUIRE_EQUAL(from_status(status::loop_detected), "508 LOOP_DETECTED");
    BOOST_REQUIRE_EQUAL(from_status(status::not_extended), "510 NOT_EXTENDED");
    BOOST_REQUIRE_EQUAL(from_status(status::network_authentication_required), "511 NETWORK_AUTHENTICATION_REQUIRED");
}

BOOST_AUTO_TEST_SUITE_END()
