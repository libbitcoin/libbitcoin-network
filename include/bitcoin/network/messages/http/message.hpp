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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_MESSAGE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_MESSAGE_HPP

#include <optional>
#include <utility>
#include <bitcoin/network/io/io.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/header.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// A body is a type with value_type, and reader and writer classes:
///   reader(header<IsRequest>&, value_type&)
///     void init(const length_type& length, code& ec)
///     size_t put(const io::const_buffer& buffer, code& ec)
///     void finish(code& ec)
///   writer(header<IsRequest>&, value_type&)
///     void init(code& ec)
///     get_buffer<io::const_buffer> get(code& ec)
/// A reader may consume less than given (need_buffer, retains the rest).
/// A writer returns no buffer when done, or a buffer and whether more follow.
using length_type = std::optional<uint64_t>;
template <class Buffer>
using get_buffer = std::optional<std::pair<Buffer, bool>>;

/// Not thread safe, non-virtual.
/// A request or response header with a body value.
template <bool IsRequest, class Body>
class message
  : public header<IsRequest>
{
public:
    using header_type = header<IsRequest>;
    using body_type = Body;
    using value_type = typename Body::value_type;

    DEFAULT_COPY_MOVE_DESTRUCT(message);

    message() NOEXCEPT = default;

    /// Construct a request.
    message(verb method, const std::string_view& target,
        uint32_t version) NOEXCEPT requires (IsRequest)
      : header_type(method, target, version)
    {
    }

    /// Construct a response.
    message(status result, uint32_t version) NOEXCEPT requires (!IsRequest)
      : header_type(result, version)
    {
    }

    header_type& base() NOEXCEPT
    {
        return *this;
    }

    const header_type& base() const NOEXCEPT
    {
        return *this;
    }

    value_type& body() NOEXCEPT
    {
        return body_;
    }

    const value_type& body() const NOEXCEPT
    {
        return body_;
    }

    /// Frame the body: chunked for http 1.1, closing the connection for 1.0,
    /// and neither for a response status that precludes a body.
    void prepare_payload() NOEXCEPT
    {
        if constexpr (!IsRequest)
        {
            if (this->bodiless())
            {
                this->chunked(false);
                this->erase(field::content_length);
                return;
            }
        }

        this->chunked(this->version() >= version_1_1);
    }

private:
    value_type body_{};
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
