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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_HEADER_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_HEADER_HPP

#include <optional>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/status.hpp>
#include <bitcoin/network/messages/http/enums/verb.hpp>
#include <bitcoin/network/messages/http/fields.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// http protocol versions.
constexpr uint32_t version_1_1 = 11;
constexpr uint32_t version_1_0 = 10;

/// Not thread safe, non-virtual.
/// The version and fields common to request and response headers.
class BCT_API header_base
  : public fields
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(header_base);

    /// The version (major * 10 + minor).
    uint32_t version() const NOEXCEPT;
    void version(uint32_t value) NOEXCEPT;

    /// The connection persists, per version and the connection field.
    bool keep_alive() const NOEXCEPT;
    void keep_alive(bool value) NOEXCEPT;

    /// The last transfer coding is chunked.
    bool chunked() const NOEXCEPT;

    /// Set (append) or clear chunked, setting removes content-length.
    void chunked(bool value) NOEXCEPT;

    /// The content-length, if the field is present and valid.
    std::optional<uint64_t> content_length() const NOEXCEPT;

    /// Set or clear content-length, either removes chunked.
    void content_length(const std::optional<uint64_t>& value) NOEXCEPT;

protected:
    header_base(uint32_t version) NOEXCEPT;

private:
    uint32_t version_;
};

template <bool IsRequest>
class header;

/// Not thread safe, non-virtual.
/// Request line and fields.
template <>
class BCT_API header<true>
  : public header_base
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(header);

    header() NOEXCEPT;
    header(verb method, const std::string_view& target,
        uint32_t version) NOEXCEPT;

    /// The method, unknown if its token is not enumerated.
    verb method() const NOEXCEPT;
    void method(verb value) NOEXCEPT;

    /// The method token.
    std::string_view method_string() const NOEXCEPT;
    void method_string(const std::string_view& value) NOEXCEPT;

    /// The request target.
    const std::string& target() const NOEXCEPT;
    void target(const std::string_view& value) NOEXCEPT;

    /// The connection closes after the message is sent.
    bool need_eof() const NOEXCEPT;

private:
    verb method_;
    std::string method_string_{};
    std::string target_{};
};

/// Not thread safe, non-virtual.
/// Status line and fields.
template <>
class BCT_API header<false>
  : public header_base
{
public:
    DEFAULT_COPY_MOVE_DESTRUCT(header);

    header() NOEXCEPT;
    header(status result, uint32_t version) NOEXCEPT;

    /// The status, unknown if its code is not enumerated.
    status result() const NOEXCEPT;
    void result(status value) NOEXCEPT;
    void result(uint32_t code) NOEXCEPT;

    /// The status code.
    uint32_t result_int() const NOEXCEPT;

    /// The reason phrase, the standard phrase of the status if not set.
    std::string reason() const NOEXCEPT;
    void reason(const std::string_view& value) NOEXCEPT;

    /// The status precludes a body.
    bool bodiless() const NOEXCEPT;

    /// The connection closes after the message is sent.
    bool need_eof() const NOEXCEPT;

private:
    uint32_t code_;
    std::string reason_{};
    std::string standard_{};
};

using request_header = header<true>;
using response_header = header<false>;

/// The request is an http 1.1 GET with connection upgrade to websocket.
BCT_API bool is_upgrade(const request_header& header) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
