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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_VARIANT_HEAD_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_VARIANT_HEAD_HPP

#include <optional>
#include <variant>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/http.hpp>

// TODO: relies on http::length_type, get_buffer.

namespace libbitcoin {
namespace network {
namespace variant {

/// Variant head<> template, analogous to the variant body<> template.
/// Allows universal request and response message types with single asio async
/// read(request) and write(response) implementations. body<> is auto-selected
/// in the read(request) through its access to the header (e.g. content-type).

// TODO: expand these variants to support:
// line framing: electrum, strantum_v1 (custom head).
// websockets framing: rest API (using beast).
// stratum_v2 framing: stratum_v2 (custom head).
// zeromq framing: rest API (custom head).

// Aliases for the two HTTP head types (with default http::fields).

template <bool IsRequest>
using http_reader = typename http::head<IsRequest>::reader;
template <bool IsRequest>
using head_reader = std::variant
<
    http_reader<IsRequest>
>;

template <bool IsRequest>
using http_writer = typename http::head<IsRequest>::writer;
template <bool IsRequest>
using head_writer = std::variant
<
    http_writer<IsRequest>
>;

template <bool IsRequest>
using http_value = typename http::head<IsRequest>::value_type;
template <bool IsRequest>
using head_value = std::variant
<
    http_value<IsRequest>
>;

/// Universal header for all known framing types.
/// Template differentiates request/response message types, though these vary
/// only due to the slight asymmetry between http request and response headers.
/// Currently only HTTP, but designed for future extension (WebSocket, etc.)
template <bool IsRequest>
struct head
{
    struct value_type
    {
        value_type() NOEXCEPT = default;

        FORWARD_VARIANT_CONSTRUCT(value_type, inner_)
        FORWARD_VARIANT_ASSIGNMENT(value_type, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, http_value<IsRequest>,  inner_)

        inline bool has_value() const NOEXCEPT
        {
            return inner_.has_value();
        }

        inline head_value<IsRequest>& value() NOEXCEPT
        {
            return inner_.value(); 
        }

        inline const head_value<IsRequest>& value() const NOEXCEPT
        {
            return inner_.value();
        }

        template <typename Inner>
        inline bool contains() const NOEXCEPT
        {
            return has_value() && std::holds_alternative<Inner>(value());
        }

        template <typename Inner>
        inline const Inner& get() const NOEXCEPT
        {
            BC_ASSERT(contains<Inner>());
            return std::get<Inner>(value());
        }

    private:
        std::optional<head_value<IsRequest>> inner_{};
    };

    class reader
    {
    public:
        using buffer_type = asio::const_buffer;

        inline explicit reader(value_type& value) NOEXCEPT
          : reader_{ to_reader(value) }
        {
        }

        inline void init(const http::length_type& length,
            boost_code& ec) NOEXCEPT;
        inline size_t put(const buffer_type& buffer, boost_code& ec) NOEXCEPT;
        inline void finish(boost_code& ec) NOEXCEPT;

    private:
        static inline head_reader<IsRequest> to_reader(
            value_type& value) NOEXCEPT
        {
            return std::visit(overload
            {
                [&](http_value<IsRequest>& value) NOEXCEPT
                {
                    return head_reader<IsRequest>{
                        std::in_place_type<http_reader<IsRequest>>, value };
                }
            }, value.value());
        }

        head_reader<IsRequest> reader_;
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = http::get_buffer<const_buffers_type>;

        inline explicit writer(value_type& value) NOEXCEPT
          : writer_(to_writer(value))
        {
        }

        inline void init(boost_code& ec) NOEXCEPT;
        inline out_buffer get(boost_code& ec) NOEXCEPT;

    private:
        static inline head_writer<IsRequest> to_writer(
            value_type& value) NOEXCEPT
        {
            return std::visit(overload
            {
                [&](http_value<IsRequest>& value) NOEXCEPT
                {
                    return head_writer<IsRequest>{
                        std::in_place_type<http_writer<IsRequest>>, value };
                }
            }, value.value());
        }

        head_writer<IsRequest> writer_;
    };
};

} // namespace variant
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <bool IsRequest>
#define CLASS head<IsRequest>

#include <bitcoin/network/impl/messages/variant/head.ipp>

#undef CLASS
#undef TEMPLATE

#endif
