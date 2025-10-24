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

        template <class Buffers>
        size_t put(const Buffers& buffers, error_code& ec) NOEXCEPT;

        void init(std::optional<uint64_t> const&, error_code& ec) NOEXCEPT;
        void finish(error_code& ec) NOEXCEPT;

    private:
        value_type& parser_;
    };

    class writer
    {
    public:
        using buffer_t = typename Parser::buffer_t;

        template <bool Request>
        explicit writer(http::header<Request>&) NOEXCEPT
        {
        }

        template <class Buffers>
        size_t put(const Buffers& buffers, error_code& ec) NOEXCEPT;

        void init(error_code& ec) NOEXCEPT;
        void finish(error_code& ec) NOEXCEPT;
        buffer_t buffer() NOEXCEPT;

    private:
        buffer_t buffer_{};
    };
};

} // namespace json
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <class Parser>

#define CLASS body<Parser>::reader
#include <bitcoin/network/impl/messages/json/body_reader.ipp>
#undef CLASS

#define CLASS body<Parser>::writer
#include <bitcoin/network/impl/messages/json/body_writer.ipp>
#undef CLASS

#undef TEMPLATE

#endif
