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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_RPC_BODY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_RPC_BODY_HPP

#include <memory>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json_body.hpp>
#include <bitcoin/network/messages/rpc/model.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

template <typename Type>
struct message_type
  : public json::json_value
{
    Type message{};
};

/// Derived boost::beast::http body for JSON-RPC messages.
/// Extends json::body with JSON-RPC validation.
template <typename Message>
struct BCT_API body
  : public json::body<message_type<Message>>
{
    using message_value = message_type<Message>;
    using base = typename json::body<message_value>;
    using value_type = base::value_type;

    class reader
      : public json::body<message_value>::reader
    {
    public:
        using reader_type = base::reader;
        using buffer_type = reader_type::buffer_type;

        inline explicit reader(value_type& value) NOEXCEPT
          : reader_type{ value }, terminated_{ true }
        {
        }

        template <bool IsRequest, class Fields>
        inline explicit reader(http::message_header<IsRequest, Fields>& header,
            value_type& value) NOEXCEPT
          : reader_type{ header, value }
        {
        }

        size_t put(const buffer_type& buffer, boost_code& ec) NOEXCEPT override;
        void finish(boost_code& ec) NOEXCEPT override;
        bool done() const NOEXCEPT override;

    private:
        const bool terminated_{};
        bool has_terminator_{};
    };

    class writer
      : public json::body<message_value>::writer
    {
    public:
        using writer_type = base::writer;
        using out_buffer = writer_type::out_buffer;

        inline explicit writer(value_type& value) NOEXCEPT
          : writer_type{ value }, terminate_{ true }
        {
        }

        template <bool IsRequest, class Fields>
        inline explicit writer(http::message_header<IsRequest, Fields>& header,
            value_type& value) NOEXCEPT
          : writer_type{ header, value }
        {
        }

        void init(boost_code& ec) NOEXCEPT override;
        out_buffer get(boost_code& ec) NOEXCEPT override;

    private:
        const bool terminate_{};
    };
};

using request_body = body<request_t>;
using request = request_body::value_type;
using request_cptr = std::shared_ptr<const request>;
using request_ptr = std::shared_ptr<request>;
using reader = request_body::reader;

using response_body = body<response_t>;
using response = response_body::value_type;
using response_cptr = std::shared_ptr<const response>;
using response_ptr = std::shared_ptr<response>;
using writer = response_body::writer;

} // namespace rpc
} // namespace network
} // namespace libbitcoin

#endif
