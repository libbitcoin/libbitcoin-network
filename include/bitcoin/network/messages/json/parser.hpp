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

#include <optional>
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
    complete,
    error_state
};

/// A minimal-copy parser for boost asio JSON-RPC v1/v2 stream parsing.
template <bool Request, bool Strict = true>
class parser
{
public:
    /// Required for body template.
    using buffer_t = string_t;

    /// Stratum v1 uses json-rpc v2 but does not require its "jsonrp" element.
    static constexpr auto require_jsonrpc_v2 = Strict;

    /// Parsed object type.
    static constexpr auto request = Request;
    static constexpr auto response = !request;
    using parsed_t = iif<request, request_t, response_t>;
    using batch_t = std::vector<parsed_t>;

    /// Constructor.
    explicit parser(json::protocol proto) NOEXCEPT
      : protocol_{ proto }
    {
    }

    /// Clear state for new parse.
    void reset() NOEXCEPT;

    /// Properties.
    bool is_done() const NOEXCEPT;
    bool has_error() const NOEXCEPT;
    error_code get_error() const NOEXCEPT;
    const batch_t& get_parsed() const NOEXCEPT;

    /// Invoke streaming parse of data.
    size_t write(std::string_view data, error_code& ec) NOEXCEPT;

protected:
    using state = parser_state;
    using view = std::string_view;
    using rpc_iterator = batch_t::iterator;
    using char_iterator = view::const_iterator;

    /// Finalize the current token, sets result elements.
    void finalize() NOEXCEPT;

    /// Validate following completion, updates state.
    void validate() NOEXCEPT;

    /// Accumulate the current character.
    void parse_character(char c) NOEXCEPT;

    /// Visitors.
    /// -----------------------------------------------------------------------

    /// State transitioners.
    void handle_initialize(char c) NOEXCEPT;
    void handle_object_start(char c) NOEXCEPT;
    void handle_key(char c) NOEXCEPT;
    void handle_value(char c) NOEXCEPT;

    /// Quoted value handlers.
    void handle_jsonrpc(char c) NOEXCEPT;
    void handle_method(char c) NOEXCEPT;
    void handle_params(char c) NOEXCEPT;
    void handle_id(char c) NOEXCEPT;
    void handle_result(char c) NOEXCEPT;
    void handle_error_start(char c) NOEXCEPT;
    void handle_error_code(char c) NOEXCEPT;
    void handle_error_message(char c) NOEXCEPT;
    void handle_error_data(char c) NOEXCEPT;

protected:
    /// Static.
    /// -----------------------------------------------------------------------
    static inline error_code parse_error() NOEXCEPT;

    static inline bool is_null(const id_t& id) NOEXCEPT;
    static inline bool is_whitespace(char c) NOEXCEPT;

    static inline bool to_number(int64_t& out, view token) NOEXCEPT;
    static inline id_t to_id(view token) NOEXCEPT;

    static inline bool increment(size_t& depth, state& status) NOEXCEPT;
    static inline bool decrement(size_t& depth, state& status) NOEXCEPT;

    static inline void consume(view& token, const char_iterator& at) NOEXCEPT;
    static inline size_t distance(const char_iterator& from,
        const char_iterator& to) NOEXCEPT;

    /// Escaping.
    /// -----------------------------------------------------------------------
    inline void consume_substitute(view& token, char c) NOEXCEPT;
    inline void consume_escaped(view& token, char c) NOEXCEPT;
    inline bool consume_escape(view& token, char c) NOEXCEPT;

    /// Versioning.
    /// -----------------------------------------------------------------------
    inline bool is_closed() const NOEXCEPT;
    inline bool is_version1() const NOEXCEPT;
    inline bool is_version2() const NOEXCEPT;
    inline bool is_terminal(char c) const NOEXCEPT;
    inline bool is_version(view token) const NOEXCEPT;

    bool batched_{};
    bool escaped_{};
    bool quoted_{};
    state state_{};
    size_t depth_{};

    char_iterator char_{};
    view key_{};
    view value_{};

    batch_t batch_{};
    result_t error_{};
    rpc_iterator parsed_{};

    const json::protocol protocol_;
};

} // namespace json
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <bool Request, bool Strict>
#define CLASS parser<Request, Strict>

#include <bitcoin/network/impl/messages/json/parser.ipp>

#undef CLASS
#undef TEMPLATE

#endif
