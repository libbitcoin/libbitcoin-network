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
    root,
    batch_start,
    request_start,
    key,
    value,
    jsonrpc,
    method,
    id,
    params,
    params_start,
    parameter_key,
    parameter_value,
    parameter,
    error_state,
    complete
};

/// A minimal-copy parser for boost asio JSON-RPC v1/v2 stream parsing.
template <bool Strict = true,
    json::version Require = json::version::any,
    bool Trace = false>
class parser
{
public:
    /// Required for body template.
    using buffer_t = string_t;

    /// Parsed object type.
    /// -----------------------------------------------------------------------
    static constexpr auto strict = Strict;
    static constexpr auto require = Require;
    static constexpr auto trace = Trace;
    using batch_t = std::vector<request_t>;

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
    using char_it = view_t::const_iterator;
    using request_it = batch_t::iterator;

    /// Statics.
    /// -----------------------------------------------------------------------
    static inline error_code failure() NOEXCEPT;
    static inline error_code incomplete() NOEXCEPT;
    static constexpr bool is_whitespace(char c) NOEXCEPT;
    static inline bool is_null_t(const id_t& id) NOEXCEPT;
    static inline bool is_numeric(char c) NOEXCEPT;
    static inline bool is_truthy(const view_t& token, char c) NOEXCEPT;
    static inline bool is_falsy(const view_t& token, char c) NOEXCEPT;
    static inline bool is_nully(const view_t& token, char c) NOEXCEPT;
    static inline bool is_error(const result_t& error) NOEXCEPT;
    static inline bool to_signed(code_t& out, const view_t& token) NOEXCEPT;
    static inline bool to_number(number_t& out, const view_t& token) NOEXCEPT;
    static inline bool toggle(bool& quoted) NOEXCEPT;
    static inline size_t distance(const char_it& from,
        const char_it& to) NOEXCEPT;

    /// Methods.
    /// -----------------------------------------------------------------------
    bool done_parsing(char c) NOEXCEPT;
    void redispatch(state transition) NOEXCEPT;
    void reset_internal() NOEXCEPT;
    void validate() NOEXCEPT;

    /// Visitors - object transitions.
    /// -----------------------------------------------------------------------
    void handle_root(char c) NOEXCEPT;
    void handle_batch_start(char c) NOEXCEPT;
    void handle_request_start(char c) NOEXCEPT;
    void handle_key(char c) NOEXCEPT;
    void handle_value(char c) NOEXCEPT;
    void handle_params(char c) NOEXCEPT;
    void handle_params_start(char c) NOEXCEPT;
    void handle_parameter_key(char c) NOEXCEPT;
    void handle_parameter_value(char c) NOEXCEPT;

    /// Visitors - quoted values.
    /// -----------------------------------------------------------------------
    void handle_jsonrpc(char c) NOEXCEPT;
    void handle_method(char c) NOEXCEPT;
    void handle_id(char c) NOEXCEPT;
    void handle_parameter(char c) NOEXCEPT;

    /// Comsuming.
    /// -----------------------------------------------------------------------
    inline bool consume_substitute(view_t& token, char c) NOEXCEPT;
    inline bool consume_escaped(view_t& token, char c) NOEXCEPT;
    inline bool consume_escape(view_t& token, char c) NOEXCEPT;
    inline size_t consume_buffer(view_t& token) NOEXCEPT;
    inline size_t consume_quoted(view_t& token) NOEXCEPT;
    inline size_t consume_char(view_t& token) NOEXCEPT;
    inline bool consume_object(view_t& token) NOEXCEPT;
    inline bool consume_array(view_t& token) NOEXCEPT;

    /// Assignment.
    /// -----------------------------------------------------------------------
    static inline const request_it add_request(batch_t& batch) NOEXCEPT;
    static inline void add_array(params_option& params) NOEXCEPT;
    static inline void add_object(params_option& params) NOEXCEPT;
    static inline bool is_array(const params_option& params) NOEXCEPT;
    static inline bool is_empty(const params_option& params) NOEXCEPT;

    /// "jsonrpc"
    inline bool assign_version(version& to, view_t& value) NOEXCEPT;

    /// "method"
    inline void assign_string(string_t& to, view_t& value) NOEXCEPT;

    /// "id"
    inline bool assign_number(id_option& to, view_t& value) NOEXCEPT;
    inline void assign_string(id_option& to, view_t& value) NOEXCEPT;
    inline void assign_null(id_option& to, view_t& value) NOEXCEPT;

    /// "params"
    template <class Type, class... Value>
    inline bool push_param(params_option& to, const view_t& key,
        Value&&... value) NOEXCEPT;
    inline bool push_array(params_option& to, view_t& key,
        view_t& value) NOEXCEPT;
    inline bool push_object(params_option& to, view_t& key,
        view_t& value) NOEXCEPT;
    inline bool push_string(params_option& to, view_t& key,
        view_t& value) NOEXCEPT;
    inline bool push_number(params_option& to, view_t& key,
        view_t& value) NOEXCEPT;
    inline bool push_boolean(params_option& to, view_t& key,
        view_t& value) NOEXCEPT;
    inline bool push_null(params_option& to, view_t& key,
        view_t& value) NOEXCEPT;

private:
    static constexpr auto false_size = view_t{ "false" }.length();
    static constexpr auto true_size = view_t{ "true" }.length();
    static constexpr auto null_size = view_t{ "null" }.length();

    // These are not thread safe.

    bool batched_{};
    batch_t batch_{};

    bool after_{};
    bool escaped_{};
    bool quoted_{};
    state state_{};
    char_it char_{};
    char_it begin_{};
    view_t key_{};
    view_t value_{};
    request_it request_{};
};

} // namespace json
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <bool Strict, json::version Require, bool Trace>
#define CLASS parser<Strict, Require, Trace>

#include <bitcoin/network/impl/messages/json/parser.ipp>
#include <bitcoin/network/impl/messages/json/parser_assign.ipp>
#include <bitcoin/network/impl/messages/json/parser_consume.ipp>
#include <bitcoin/network/impl/messages/json/parser_statics.ipp>
#include <bitcoin/network/impl/messages/json/parser_object.ipp>
#include <bitcoin/network/impl/messages/json/parser_value.ipp>

#undef CLASS
#undef TEMPLATE

#endif
