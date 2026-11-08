/**
 * Copyright (c) 2011-2026 libbitcoin developers
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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_BODY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_BODY_HPP

#include <optional>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/http.hpp>
#include <bitcoin/network/messages/indirect.hpp>
#include <bitcoin/network/messages/json_body.hpp>
#include <bitcoin/network/messages/peer/body.hpp>
#include <bitcoin/network/messages/rpc/rpc.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using empty_reader = http::empty_body::reader;
using data_reader = http::chunk_body::reader;
using file_reader = http::file_body::reader;
using span_reader = http::span_body::reader;
using buffer_reader = http::buffer_body::reader;
using string_reader = http::string_body::reader;
using json_reader = http::json_body::reader;
using peer_reader = messages::peer::body::reader;
using body_reader = std::variant
<
    std::monostate, //     1 byte
    empty_reader,   //     1 bytes
    data_reader,    //     8 bytes
    file_reader,    //     8 bytes
    span_reader,    //     8 bytes
    buffer_reader,  //     8 bytes
    string_reader,  //     8 bytes
    peer_reader,    //    32 bytes
    json_reader,    //   320 bytes!
    rpc::reader     //   328 bytes!
>;

using empty_writer = http::empty_body::writer;
using data_writer = http::chunk_body::writer;
using file_writer = http::file_body::writer;
using span_writer = http::span_body::writer;
using buffer_writer = http::buffer_body::writer;
using stream_writer = http::stream_body::writer;
using string_writer = http::string_body::writer;
using json_writer = http::json_body::writer;
using peer_writer = messages::peer::body::writer;
using body_writer = std::variant
<
    std::monostate, //     1 byte
    empty_writer,   //     1 byte
    data_writer,    //     8 bytes
    file_writer,    //     8 bytes
    span_writer,    //     8 bytes
    buffer_writer,  //    16 bytes
    stream_writer,  //    32 bytes
    string_writer,  //     8 bytes
    peer_writer,    //    32 bytes
    json_writer,    //   136 bytes!
    rpc::writer,    //   144 bytes!
    rpc::notifier   //   144 bytes!
>;

using empty_value = http::empty_body::value_type;
using data_value = http::chunk_body::value_type;
using file_value = http::file_body::value_type;
using span_value = http::span_body::value_type;
using buffer_value = http::buffer_body::value_type;
using stream_value = http::stream_body::value_type;
using string_value = http::string_body::value_type;
using json_value = http::json_body::value_type;
using peer_value = messages::peer::body::value_type;
using stream_indirect = indirect<stream_value>;
using rpc_request = indirect<rpc::request>;
using rpc_response = indirect<rpc::response>;
using body_value = std::variant
<
    empty_value,    //   1 byte
    data_value,     //  24 bytes
    file_value,     //  16 bytes
    span_value,     //  16 bytes
    buffer_value,   //  24 bytes
    stream_indirect, //  8 bytes (80 indirect)
    string_value,   //  32 bytes
    json_value,     //  56 bytes
    peer_value,     // 256 bytes
    rpc_request,    //   8 bytes (240 indirect)
    rpc_response    //   8 bytes (328 indirect)
>;

/// body template for all known message types.
/// This encapsulates a variant of supported body types, selects a type upon
/// reader or writer construction, and then passes all calls through to it.
struct BCT_API body
{
    /// No size(), forces chunked encoding for all types.
    /// The pass-thru body(), reader populates in construct.
    struct value_type
    {
        /// Set to change reader to plain json (vs. json-rpc).
        /// Writer is determined by assigned body type.
        bool plain_json{};

        /// Set to read a post body as json-rpc regardless of content type.
        bool rpc_post{};

        /// Allow default construct (empty optional).
        value_type() NOEXCEPT = default;

        /// Forwarding constructors for in-place variant construction.
        FORWARD_VARIANT_CONSTRUCT(value_type, inner_)
        FORWARD_VARIANT_ASSIGNMENT(value_type, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, empty_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, data_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, file_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, span_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, buffer_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, string_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, json_value, inner_)
        FORWARD_ALTERNATIVE_VARIANT_ASSIGNMENT(value_type, peer_value, inner_)

        /// The indirect kinds are emplaced explicitly.
        inline value_type& operator=(stream_value&& value) NOEXCEPT
        {
            inner_.emplace(std::in_place_type<stream_indirect>, std::move(value));
            return *this;
        }

        inline value_type& operator=(rpc::request&& value) NOEXCEPT
        {
            inner_.emplace(std::in_place_type<rpc_request>, std::move(value));
            return *this;
        }

        inline value_type& operator=(rpc::response&& value) NOEXCEPT
        {
            inner_.emplace(std::in_place_type<rpc_response>, std::move(value));
            return *this;
        }

        inline bool has_value() const NOEXCEPT
        {
            return inner_.has_value();
        }

        inline body_value& value() NOEXCEPT
        {
            return inner_.value();
        }

        inline const body_value& value() const NOEXCEPT
        {
            return inner_.value();
        }

        template <typename Inner>
        inline bool contains() const NOEXCEPT
        {
            return has_value() && std::holds_alternative<stored<Inner>>(value());
        }

        template <typename Inner>
        inline const Inner& get() const NOEXCEPT
        {
            BC_ASSERT(contains<Inner>());
            if constexpr (is_indirect<Inner>)
                return *std::get<stored<Inner>>(value());
            else
                return std::get<Inner>(value());
        }

        template <typename Inner>
        inline Inner& get() NOEXCEPT
        {
            BC_ASSERT(contains<Inner>());
            if constexpr (is_indirect<Inner>)
                return *std::get<stored<Inner>>(value());
            else
                return std::get<Inner>(value());
        }

    private:
        template <typename Inner>
        static constexpr bool is_indirect =
            std::is_same_v<Inner, stream_value> ||
            std::is_same_v<Inner, rpc::request> ||
            std::is_same_v<Inner, rpc::response>;

        template <typename Inner>
        using stored = std::conditional_t<is_indirect<Inner>,
            indirect<Inner>, Inner>;

        std::optional<body_value> inner_{};
    };

    class BCT_API reader
    {
    public:
        using buffer_type = io::const_buffer;

        /// Streamed reads may finish before the message is complete.
        template <bool IsRequest>
        inline explicit reader(http::header<IsRequest>& header,
            value_type& value, bool streamed=false) NOEXCEPT
          : header_{ header }, value_{ value }, reader_{ std::monostate{} },
            streamed_{ streamed }
        {
        }

        void init(const http::length_type& length, code& ec) NOEXCEPT;
        size_t put(const buffer_type& buffer, code& ec) NOEXCEPT;
        void finish(code& ec) NOEXCEPT;

    protected:
        // The reader only reads requests, and http::fields are required.
        using header = http::request_header;

        /// Select reader based on content-type header.
        inline void assign_reader(header& header, value_type& value) NOEXCEPT
        {
            // Allows for preselection of reader for non-http reads.
            if (!value_.has_value())
            {
                if (value_.rpc_post && (header.method() == http::verb::post))
                {
                    value = rpc::request{};
                }
                else
                {
                    switch (http::content_media_type(header))
                    {
                        case http::media_type::application_json:
                            if (value_.plain_json)
                                value = json_value{};
                            else
                                value = rpc::request{};
                            break;
                        case http::media_type::text_plain:
                            value = string_value{};
                            break;
                        case http::media_type::application_octet_stream:
                            if (http::has_attachment(header))
                                value = file_value{};
                            else
                                value = data_value{};
                            break;
                        default:
                            value = empty_value{};
                    }
                }
            }

            std::visit(overload
            {
                // These are not selectable above.
                [&](std::monostate&) NOEXCEPT {},
                [&](span_value&) NOEXCEPT {},
                [&](buffer_value&) NOEXCEPT {},
                [&](stream_indirect&) NOEXCEPT {},
                [&](rpc_response&) NOEXCEPT {},

                [&](empty_value& value) NOEXCEPT
                {
                    reader_.emplace<empty_reader>(header, value);
                },
                [&](data_value& value) NOEXCEPT
                {
                    reader_.emplace<data_reader>(header, value);
                },
                [&](file_value& value) NOEXCEPT
                {
                    reader_.emplace<file_reader>(header, value);
                },
                [&](string_value& value) NOEXCEPT
                {
                    reader_.emplace<string_reader>(header, value);
                },
                [&](json_value& value) NOEXCEPT
                {
                    // json_reader not copy or assignable (by contained parser).
                    reader_.emplace<json_reader>(header, value);
                },
                [&](peer_value& value) NOEXCEPT
                {
                    // Selectable only by preselection (non-http reads).
                    reader_.emplace<peer_reader>(header, value);
                },
                [&](rpc_request& value) NOEXCEPT
                {
                    // json_reader not copy or assignable (by contained parser).
                    reader_.emplace<rpc::reader>(header, *value);
                }
            }, value.value());
        }

    protected:
        header& header_;
        value_type& value_;
        body_reader reader_;
        const bool streamed_;
    };

    class BCT_API writer
    {
    public:
        using const_buffers_type = io::const_buffer;
        using out_buffer = http::get_buffer<const_buffers_type>;

        template <bool IsRequest>
        inline explicit writer(http::header<IsRequest>& header,
            value_type& value) NOEXCEPT
          : writer_{ to_writer(header, value) }
        {
        }

        inline bool binary() const NOEXCEPT
        {
            return std::visit(overload
            {
                [&](const std::monostate&) NOEXCEPT { return false; },
                [&](const empty_writer&)   NOEXCEPT { return false; },
                [&](const data_writer&)    NOEXCEPT { return true; },
                [&](const file_writer&)    NOEXCEPT { return true; },
                [&](const span_writer&)    NOEXCEPT { return true; },
                [&](const buffer_writer&)  NOEXCEPT { return true; },
                [&](const stream_writer&)  NOEXCEPT { return true; },
                [&](const string_writer&)  NOEXCEPT { return false; },
                [&](const peer_writer&)    NOEXCEPT { return true; },
                [&](const json_writer&)    NOEXCEPT { return false; },
                [&](const rpc::writer&)    NOEXCEPT { return false; },
                [&](const rpc::notifier&)  NOEXCEPT { return false; }
            }, writer_);
        }

        void init(code& ec) NOEXCEPT;
        out_buffer get(code& ec) NOEXCEPT;

    protected:
        /// Create writer matching the caller-defined body inner variant type.
        template <class Header>
        static inline body_writer to_writer(Header& header,
            value_type& value) NOEXCEPT
        {
            // Caller should have set optional<>, otherwise set to empty_value.
            if (!value.has_value())
                value = empty_value{};

            return std::visit(overload
            {
                [&](empty_value& value) NOEXCEPT
                {
                    return body_writer{ empty_writer{ header, value } };
                },
                [&](data_value& value) NOEXCEPT
                {
                    return body_writer{ data_writer{ header, value } };
                },
                [&](file_value& value) NOEXCEPT
                {
                    return body_writer{ file_writer{ header, value } };
                },
                [&](span_value& value) NOEXCEPT
                {
                    return body_writer{ span_writer{ header, value } };
                },
                [&](buffer_value& value) NOEXCEPT
                {
                    return body_writer{ buffer_writer{ header, value } };
                },
                [&](stream_indirect& value) NOEXCEPT
                {
                    return body_writer{ stream_writer{ header, *value } };
                },
                [&](string_value& value) NOEXCEPT
                {
                    return body_writer{ string_writer{ header, value } };
                },
                [&](json_value& value) NOEXCEPT
                {
                    // json_writer is not movable (by contained serializer).
                    // So requires in-place construction for variant populate.
                    return body_writer
                    {
                        std::in_place_type<json_writer>, header, value
                    };
                },
                [&](peer_value& value) NOEXCEPT
                {
                    return body_writer{ peer_writer{ header, value } };
                },
                [&](rpc_response& value) NOEXCEPT
                {
                    // rpc::writer is not movable (by contained serializer).
                    // So requires in-place construction for variant populate.
                    return body_writer
                    {
                        std::in_place_type<rpc::writer>, header, *value
                    };
                },
                [&](rpc_request& value) NOEXCEPT
                {
                    // rpc::writer is not movable (by contained serializer).
                    // So requires in-place construction for variant populate.
                    return body_writer
                    {
                        std::in_place_type<rpc::notifier>, header, *value
                    };
                }
            }, value.value());
        }

    private:
        body_writer writer_;
    };
};

using request = message<true, http::body>;
using request_cptr = std::shared_ptr<const request>;
using request_ptr = std::shared_ptr<request>;

using response = message<false, http::body>;
using response_cptr = std::shared_ptr<const response>;
using response_ptr = std::shared_ptr<response>;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
