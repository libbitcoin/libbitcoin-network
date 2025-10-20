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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_PARSER_HPP

#include <string_view>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

/// Parser states (unconditional by template).
enum class parser_state
{
    initial,
    object_start,
    key,
    value,
    jsonrpc,
    method,
    params,
    id,
    result,
    error_start,
    error_code,
    error_message,
    error_data,
    error_state,
    complete
};

/// A minimal-copy parser for boost asio JSON-RPC v1/v2 stream parsing.
template <
    bool Request,
    bool Strict = true,
    json::version Require = json::version::any>
class parser
{
public:
    /// Required for body template.
    using buffer_t = string_t;

    /// Parsed object type.
    /// -----------------------------------------------------------------------
    static constexpr auto strict = Strict;
    static constexpr auto require = Require;
    static constexpr auto request = Request;
    static constexpr auto response = !request;
    using parsed_t = iif<request, request_t, response_t>;
    using batch_t = std::vector<parsed_t>;

    /// Properties.
    /// -----------------------------------------------------------------------

    /// Means that parse is successful and complete.
    bool is_done() const NOEXCEPT;

    /// Implies that get_parsed() may be empty.
    bool has_error() const NOEXCEPT;

    /// Returns success in case of incomplete parse.
    error_code get_error() const NOEXCEPT;

    /// May be empty if !is_done().
    const batch_t& get_parsed() const NOEXCEPT;

    /// Methods.
    /// -----------------------------------------------------------------------
    size_t write(const std::string_view& data) NOEXCEPT;
    void reset() NOEXCEPT;

protected:
    using state = parser_state;
    using view_t = std::string_view;
    using parse_it = batch_t::iterator;
    using char_it = view_t::const_iterator;

    /// Statics.
    /// -----------------------------------------------------------------------
    static inline error_code failure() NOEXCEPT;
    static inline error_code incomplete() NOEXCEPT;
    static constexpr bool is_whitespace(char c) NOEXCEPT;
    static inline bool is_null_t(const id_t& id) NOEXCEPT;
    static inline bool is_numeric(char c) NOEXCEPT;
    static inline bool is_nullic(const view_t& token, char c) NOEXCEPT;
    static inline bool is_error(const result_t& error) NOEXCEPT;
    static inline bool to_signed(code_t& out, const view_t& token) NOEXCEPT;
    static inline bool to_double(double& out, const view_t& token) NOEXCEPT;
    static inline bool toggle(bool& quoted) NOEXCEPT;
    static inline bool increment(size_t& depth, state& status) NOEXCEPT;
    static inline bool decrement(size_t& depth, state& status) NOEXCEPT;
    static inline size_t distance(const char_it& from,
        const char_it& to) NOEXCEPT;

    /// Methods.
    /// -----------------------------------------------------------------------
    bool done_parsing(char c) NOEXCEPT;
    void validate() NOEXCEPT;

    /// Visitors - object transitions.
    /// -----------------------------------------------------------------------
    void handle_initialize(char c) NOEXCEPT;
    void handle_object_start(char c) NOEXCEPT;
    void handle_key(char c) NOEXCEPT;
    void handle_value(char c) NOEXCEPT;

    /// Visitors - quoted values.
    /// -----------------------------------------------------------------------
    void handle_jsonrpc(char c) NOEXCEPT;
    void handle_method(char c) NOEXCEPT;
    void handle_params(char c) NOEXCEPT;
    void handle_id(char c) NOEXCEPT;
    void handle_result(char c) NOEXCEPT;
    void handle_error_message(char c) NOEXCEPT;
    void handle_error_data(char c) NOEXCEPT;

    /// Visitors - unquoted values.
    /// -----------------------------------------------------------------------
    void handle_error_start(char c) NOEXCEPT;
    void handle_error_code(char c) NOEXCEPT;

    /// Comsuming.
    /// -----------------------------------------------------------------------
    inline bool consume_substitute(view_t& token, char c) NOEXCEPT;
    inline bool consume_escaped(view_t& token, char c) NOEXCEPT;
    inline bool consume_escape(view_t& token, char c) NOEXCEPT;
    inline size_t consume_char(view_t& token) NOEXCEPT;

    /// Assignment.
    /// -----------------------------------------------------------------------
    inline void assign_error(error_option& to, result_t& from) NOEXCEPT;
    inline void assign_value(value_option& to, view_t& from) NOEXCEPT;
    inline void assign_string(string_t& to, view_t& from) NOEXCEPT;
    inline bool assign_version(version& to, view_t& from) NOEXCEPT;

    inline void assign_string_id(id_t& to, view_t& from) NOEXCEPT;
    inline bool assign_numeric_id(code_t& to, view_t& from) NOEXCEPT;
    inline bool assign_numeric_id(id_t& to, view_t& from) NOEXCEPT;
    inline bool assign_unquoted_id(id_t& to, view_t& from) NOEXCEPT;
    inline void assign_null_id(id_t& to, view_t& from) NOEXCEPT;

private:
    static constexpr auto null_size = view_t{ "null" }.length();
    static version to_version(const view_t& token) NOEXCEPT;

    // Add a new parsed element to the batch and return its iterator.
    inline const parse_it add_remote_procedure_call() NOEXCEPT;

    // These are not thread safe.
    bool batched_{};
    bool escaped_{};
    bool quoted_{};
    state state_{};
    size_t depth_{};

    char_it char_{};
    view_t key_{};
    view_t value_{};

    batch_t batch_{};
    result_t error_{};
    parse_it parsed_{};
};

} // namespace json
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <bool Request, bool Strict, json::version Require>
#define CLASS parser<Request, Strict, Require>

#define ASSIGN_REQUEST(kind, to, from) \
{ \
    if constexpr (request) { assign_##kind(to, from); } \
    else { state_ = state::error_state; } \
}

#define ASSIGN_RESPONSE(kind, to, from) \
{ \
    if constexpr (response) { assign_##kind(to, from); } \
    else { state_ = state::error_state; } \
}

#include <bitcoin/network/impl/messages/json/parser.ipp>
#include <bitcoin/network/impl/messages/json/parser_assign.ipp>
#include <bitcoin/network/impl/messages/json/parser_consume.ipp>
#include <bitcoin/network/impl/messages/json/parser_statics.ipp>
#include <bitcoin/network/impl/messages/json/parser_object.ipp>
#include <bitcoin/network/impl/messages/json/parser_value.ipp>

#undef ASSIGN_REQUEST
#undef ASSIGN_RESPONSE

#undef CLASS
#undef TEMPLATE

#endif
