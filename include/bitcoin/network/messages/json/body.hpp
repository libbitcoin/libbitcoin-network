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
#include <bitcoin/network/messages/json/parser.hpp>
#include <bitcoin/network/messages/json/serializer.hpp>
#include <bitcoin/network/messages/json/types.hpp>

namespace libbitcoin {
namespace network {
namespace json {

/// boost::beast body template for JSON-RPC messages.
template <class Parser, class Serializer>
struct body
{
    /// Required by boost::beast.
    using value_type = Parser::value_type;

    /// Required: reader for incoming data.
    class reader
    {
    public:
        /// Called once per message, header is mutable.
        template <bool IsRequest>
        explicit reader(http::header<IsRequest>&, value_type& body) NOEXCEPT
          : body_{ body }
        {
        }

        /// Called once at start of body deserialization.
        void init(const std::optional<uint64_t>& length,
            error_code& ec) NOEXCEPT;

        /// Bytes are consumed from buffers and parsed into json object.
        /// Called zero or more times with incoming (from wire) buffers.
        template <class ConstBufferSequence>
        size_t put(const ConstBufferSequence& buffers, error_code& ec) NOEXCEPT;

        /// Called once at the end of body deserialization, signals completion.
        void finish(error_code& ec) NOEXCEPT;

    protected:
        Parser parser_{};
        value_type& body_;
    };

    /// Required: writer for outgoing data.
    class writer
    {
    public:
        /// Called once per message, header is mutable.
        template <bool IsRequest>
        explicit writer(http::header<IsRequest>&,
            const value_type& body) NOEXCEPT
          : body_ { body }
        {
        }

        /// Called once at start of message serialization.
        void init(error_code& ec) NOEXCEPT;

        /// Bytes are serialized from json object to local buffer.
        /// Called once at the start of body serialization, after headers.
        void finish(error_code& ec) NOEXCEPT;

        /// Called one or more times with outgoing (to wire) buffers.
        template <class ConstBufferSequence>
        size_t get(ConstBufferSequence& buffers, error_code& ec) NOEXCEPT;

    protected:
        std::string buffer_{};
        const value_type& body_;
    };
};


} // namespace json
} // namespace network
} // namespace libbitcoin

#define TEMPLATE template <class Parser, class Serializer>

#define CLASS body<Parser, Serializer>::reader
#include <bitcoin/network/impl/messages/json/body_reader.ipp>
#undef CLASS

#define CLASS body<Parser, Serializer>::writer
#include <bitcoin/network/impl/messages/json/body_writer.ipp>
#undef CLASS

#undef TEMPLATE

#endif
