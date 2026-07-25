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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_FILE_BODY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_FILE_BODY_HPP

#include <memory>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {


/// BOOST_BEAST_FILE_BUFFER_SIZE reduced from 4k to 1k in beast.hpp.
/// beast_body::writer would eat up 1k in variant so wrap in this custom body.
struct file_body
{
    using beast_body = boost::beast::http::file_body;
    using value_type = beast_body::value_type;
    using reader = beast_body::reader;

    struct writer
    {
        using const_buffers_type = asio::const_buffer;
        using out_buffer = http::get_buffer<const_buffers_type>;

        template <bool IsRequest, class Fields>
        explicit writer(http::message_header<IsRequest, Fields>& header,
            beast_body::value_type& value) NOEXCEPT
            BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
          : writer_(std::make_unique<beast_body::writer>(header, value))
            BC_POP_WARNING()
        {
        }

        inline void init(boost_code& ec) NOEXCEPT
        {
            try
            {
                writer_->init(ec);
            }
            catch (...)
            {
                using namespace error;
                ec = to_http_code(http_error_t::end_of_stream);
            }
        }

        inline out_buffer get(boost_code& ec) NOEXCEPT
        {
            try
            {
                return writer_->get(ec);
            }
            catch (...)
            {
                using namespace error;
                ec = to_http_code(http_error_t::end_of_stream);
                return {};
            }
        }

    private:
        std::unique_ptr<beast_body::writer> writer_;
    };

    static inline uint64_t size(const value_type& body) NOEXCEPT
    {
        return beast_body::size(body);
    }
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
