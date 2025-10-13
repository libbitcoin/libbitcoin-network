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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_JSON_BODY_HPP

#include <optional>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/error.hpp>
#include <bitcoin/network/messages/json/parser.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

using error_code = error::boost_code;

template <class Parser>
struct body
{
    /// value_type is required for boost integration.
    using value_type = Parser;
    using buffer_t = typename Parser::buffer_t;
    using char_t = typename buffer_t::value_type;

    class reader
    {
    public:
        template <bool Request>
        explicit reader(http::header<Request>&, value_type& parser) NOEXCEPT
          : parser_{ parser }
        {
        }

        void init(std::optional<std::uint64_t> const&, error_code& ec) NOEXCEPT
        {
            ////BC_ASSERT(parser_.is_done());
            parser_.reset();
            ec.clear();
        }

        template <class Buffers>
        std::size_t put(const Buffers& buffers, error_code& ec) NOEXCEPT
        {
            ////BC_ASSERT(!parser_.is_done());

            // Prioritize existing parser error.
            if ((ec = parser_.get_error()))
                return {};

            // Already complete implies an error.
            if (parser_.is_done())
            {
                using namespace boost::system::errc;
                ec = make_error_code(protocol_error);
                return {};
            }

            std::size_t added{};
            for (auto const& buffer: buffers)
            {
                using namespace boost::asio;
                const auto size = buffer_size(buffer);
                const auto data = buffer_cast<const char_t*>(buffer);
                added += parser_.write({ data, size }, ec);
                if (ec || parser_.is_done())
                    break;
            }

            return added;
        }

        void finish(error_code& ec) NOEXCEPT
        {
            // Prioritize existing parser error.
            if ((ec = parser_.get_error()))
                return;

            // Premature completion implies an error.
            if (!parser_.is_done())
            {
                using namespace boost::system::errc;
                ec = make_error_code(protocol_error);
            }
        }

    private:
        value_type& parser_;
    };

    class writer
    {
    public:
        using buffer_t = typename Parser::buffer_t;

        template <bool Request>
        explicit writer(http::header<Request>&) NOEXCEPT {}

        void init(error_code& ec) NOEXCEPT
        {
            buffer_.clear();
            ec.clear();
        }

        template <class Buffers>
        std::size_t put(const Buffers& buffers, error_code& ec) NOEXCEPT
        {
            std::size_t added{};
            for (auto const& buffer: buffers)
            {
                using namespace boost::asio;
                const auto size = buffer_size(buffer);
                const auto data = buffer_cast<const char_t*>(buffer);
                buffer_.append(data, size);
                added += size;
            }

            ec.clear();
            return added;
        }

        void finish(error_code& ec) NOEXCEPT
        {
            // Nothing written to the response implies an error.
            if (buffer_.empty())
            {
                using namespace boost::system::errc;
                ec = make_error_code(protocol_error);
            }
        }

        buffer_t buffer() NOEXCEPT
        {
            return buffer_;
        }

    private:
        buffer_t buffer_{};
    };
};

} // namespace json
} // namespace network
} // namespace libbitcoin

#endif
