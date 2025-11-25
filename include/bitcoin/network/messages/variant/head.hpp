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

using request_reader = boost::beast::http::parser<true, http::empty_body>;
using response_reader = boost::beast::http::parser<false, http::empty_body>;
using head_reader = std::variant
<
    request_reader,
    response_reader
>;

using request_writer = boost::beast::http::serializer<true, http::empty_body>;
using response_writer = boost::beast::http::serializer<false, http::empty_body>;
using head_writer = std::variant
<
    request_writer,
    response_writer
>;

using request_header = boost::beast::http::header<true, boost::beast::http::fields>;
using response_header = boost::beast::http::header<false, boost::beast::http::fields>;
using head_value = std::variant
<
    request_header,
    response_header
>;

using empty_request = boost::beast::http::message<true, http::empty_body>;
using empty_response = boost::beast::http::message<false, http::empty_body>;

/// header template for all known message types.
/// Request head is selected by reader and response by writer (i.e. channel).
struct BCT_API head
{
    struct head_type
    {
        head_type() NOEXCEPT = default;

        FORWARD_VARIANT_CONSTRUCT(head_type, inner_)
        FORWARD_VARIANT_ASSIGNMENT(head_type, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(head_type, request_header, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(head_type, response_header, inner_)

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

        template<typename Inner>
        inline bool contains() const NOEXCEPT
        {
            return has_value() && std::holds_alternative<Inner>(value());
        }

        template<typename Inner>
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

        inline explicit reader(head_type& value) NOEXCEPT
          : reader_{ to_reader(value) }, value_{ value }
        {
        }

        void init(const http::length_type&, boost_code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, boost_code& ec) NOEXCEPT;
        void finish(boost_code& ec) NOEXCEPT;

    protected:
        inline static head_reader to_reader(head_type& value) NOEXCEPT
        {
            // Caller should have set optional<>, otherwise set invalid read.
            if (!value.has_value())
                value = response_header{};

            return std::visit(overload
            {
                [&](request_header&) NOEXCEPT
                {
                    return head_reader{ std::in_place_type<request_reader> };
                },
                [&](response_header&) NOEXCEPT
                {
                    // Server doesn't read responses.
                    return head_reader{ std::in_place_type<response_reader> };
                }
            }, value.value());
        }

    private:
        head_reader reader_;
        head_type& value_;
    };

    class writer
    {
    public:
        using const_buffers_type = asio::const_buffer;
        using out_buffer = http::get_buffer<const_buffers_type>;

        inline explicit writer(head_type& value) NOEXCEPT
          : writer_{ to_writer(value) }
        {
        }

        void init(boost_code& ec) NOEXCEPT;
        out_buffer get(boost_code& ec) NOEXCEPT;

    protected:
        /// Create writer matching caller-defined header inner variant type.
        inline static head_writer to_writer(head_type& value) NOEXCEPT
        {
            // Caller should have set optional<>, otherwise set invalid write.
            if (!value.has_value())
                value = response_header{};

            return std::visit(overload
            {
                [&] (request_header& value) NOEXCEPT
                {
                    // Server doesn't write requests.
                    return head_writer{ request_writer{ 
                        empty_request{ std::move(value) } } };
                },
                [&](response_header& value) NOEXCEPT
                {
                    return head_writer{ response_writer{ 
                        empty_response{ std::move(value) } } };
                }
            }, value.value());
        }

    private:
        head_writer writer_;
    };
};

} // namespace variant
} // namespace network
} // namespace libbitcoin

#endif
