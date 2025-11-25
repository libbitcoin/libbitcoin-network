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
/// TODO: move out of http namespace once generalized.

// TODO: expand these variants to support:
// http framing: request, response [asymmetrical] (using beast).
// line framing: electrum, strantum_v1 (custom head).
// websockets framing: rest API (using beast).
// stratum_v2 framing: stratum_v2 (custom head).
// zeromq framing: rest API (custom head).

// Aliases for the two concrete HTTP head types
using request_head  = http::head<true>;
using response_head = http::head<false>;

using request_reader  = request_head::reader;
using response_reader = response_head::reader;
using head_reader = std::variant
<
    request_reader,
    response_reader
>;

using request_writer  = request_head::writer;
using response_writer = response_head::writer;
using head_writer = std::variant
<
    request_writer,
    response_writer
>;

using request_value  = request_head::value_type;
using response_value = response_head::value_type;
using head_value = std::variant
<
    request_value,
    response_value
>;

/// Universal header for all known framing types.
/// Currently only HTTP, but designed for future extension (WebSocket, etc.)
struct BCT_API head
{
    struct value_type
    {
        value_type() NOEXCEPT = default;

        FORWARD_VARIANT_CONSTRUCT(value_type, inner_)
        FORWARD_VARIANT_ASSIGNMENT(value_type, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, request_value,  inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, response_value, inner_)

        inline bool has_value() const NOEXCEPT
        {
            return inner_.has_value();
        }

        inline head_value& value() NOEXCEPT
        {
            return inner_.value(); 
        }

        inline const head_value& value() const NOEXCEPT
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
        std::optional<head_value> inner_{};
    };

    class reader
    {
    public:
        using buffer_type = asio::const_buffer;

        inline explicit reader(value_type& value) NOEXCEPT
          : reader_{ to_reader(value) }
        {
        }

        void init(const http::length_type& length, boost_code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, boost_code& ec) NOEXCEPT;
        void finish(boost_code& ec) NOEXCEPT;

    private:
        static head_reader to_reader(value_type& value) NOEXCEPT
        {
            return std::visit(overload
            {
                [&](request_value& value) NOEXCEPT
                {
                    return head_reader{ std::in_place_type<request_reader>,
                        value };
                },
                [&](response_value& value) NOEXCEPT
                {
                    return head_reader{ std::in_place_type<response_reader>,
                        value };
                }
            }, value.value());
        }

        head_reader reader_;
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

        void init(boost_code& ec) NOEXCEPT;
        out_buffer get(boost_code& ec) NOEXCEPT;

    private:
        static head_writer to_writer(value_type& value) NOEXCEPT
        {
            return std::visit(overload
            {
                [&](request_value& value) NOEXCEPT
                {
                    return head_writer{ std::in_place_type<request_writer>,
                        value };
                },
                [&](response_value& value) NOEXCEPT
                {
                    return head_writer{ std::in_place_type<response_writer>,
                        value };
                }
            }, value.value());
        }

        head_writer writer_;
    };
};

} // namespace variant
} // namespace network
} // namespace libbitcoin

#endif
