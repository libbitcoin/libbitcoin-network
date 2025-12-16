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

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/json/body.hpp>
#include <bitcoin/network/rpc/model.hpp>

namespace libbitcoin {
namespace network {
namespace rpc {

/// Derived boost::beast::http body for JSON-RPC messages.
/// Extends json::body with JSON-RPC validation.
struct BCT_API body
  : public json::body
{
    struct value_type
      : json::body::value_type
    {
        rpc::request_t request{};
        rpc::response_t response{};
    };

    class reader
      : public json::body::reader
    {
    public:
        inline explicit reader(value_type& value) NOEXCEPT
          : json::body::reader{ value }, terminated_{ true }
        {
        }

        template <bool IsRequest, class Fields>
        inline explicit reader(http::message_header<IsRequest, Fields>& header,
            value_type& value) NOEXCEPT
          : json::body::reader{ header, value }
        {
        }

        size_t put(const buffer_type& buffer, boost_code& ec) NOEXCEPT override;
        void finish(boost_code& ec) NOEXCEPT override;
        bool is_done() const NOEXCEPT;

    private:
        const bool terminated_{};
        bool has_terminator_{};
    };

    class writer
      : public json::body::writer
    {
    public:
        inline explicit writer(value_type& value) NOEXCEPT
          : json::body::writer{ value }, terminate_{ true }
        {
        }

        template <bool IsRequest, class Fields>
        inline explicit writer(http::message_header<IsRequest, Fields>& header,
            value_type& value) NOEXCEPT
          : json::body::writer{ header, value }
        {
        }

        void init(boost_code& ec) NOEXCEPT override;
        out_buffer get(boost_code& ec) NOEXCEPT override;

    private:
        const bool terminate_{};
        boost::json::value temp_json_{};
    };
};

} // namespace rpc
} // namespace network
} // namespace libbitcoin

namespace libbitcoin {
namespace network {
namespace http {
    
using terminated_body = rpc::body;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
