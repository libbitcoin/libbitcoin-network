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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_CODINGS_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_CODINGS_HPP

#include <variant>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/enums/coding.hpp>
#include <bitcoin/network/messages/http/fields.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// The codings applied to a body, each in order of application.
struct codings
{
    /// Content codings (rfc9110 8.4), excluding identity.
    std::vector<coding> content{};

    /// Transfer codings (rfc9112 7), excluding chunked.
    std::vector<coding> transfer{};

    /// No coding is applied.
    bool empty() const NOEXCEPT
    {
        return content.empty() && transfer.empty();
    }
};

/// The codings named by the header, ec set (bad_content_encoding or
/// bad_transfer_encoding) if any is unknown.
BCT_API codings to_codings(const fields& header, code& ec) NOEXCEPT;

/// Not thread safe.
/// Encoder of a body written in parts, by its codings, content then transfer.
class BCT_API encoder
{
public:
    encoder(const codings& value) NOEXCEPT;

    /// Encode data, appending completed output to out.
    void write(system::data_chunk& out,
        const system::data_slice& data) NOEXCEPT;

    /// Encode the remainder and end the body, appending to out.
    void flush(system::data_chunk& out) NOEXCEPT;

private:
    using stage = std::variant<system::gzip_deflater, system::zlib_deflater>;

    std::vector<stage> stages_{};
};

/// Not thread safe.
/// Decoder of a body received in parts, by its codings, transfer then content,
/// each in reverse.
class BCT_API decoder
{
public:
    /// The decoded body is limited to limit bytes (zero is unlimited).
    decoder(const codings& value, size_t limit) NOEXCEPT;

    /// Decode from in, appending to out, until in is consumed or out has
    /// grown by at least step (zero is unlimited), setting the bytes of in
    /// consumed. Sets ec to bad_transfer_encoding or bad_content_encoding if
    /// a coding is invalid, or body_limit if the decoded body exceeds limit.
    void write(system::data_chunk& out, size_t& consumed,
        const system::data_slice& in, size_t step, code& ec) NOEXCEPT;

    /// Every coding has ended, with no input remaining.
    bool done() const NOEXCEPT;

private:
    using coder = std::variant<system::gzip_inflater, system::zlib_inflater>;
    struct stage
    {
        coder value;
        error::http_error_t fault;
        system::data_chunk input{};
    };

    std::vector<stage> stages_{};
    size_t limit_;
    size_t total_{};
};

/// The body encoded by the codings, content then transfer.
BCT_API system::data_chunk encode(const system::data_slice& body,
    const codings& value) NOEXCEPT;

/// The body decoded by the codings, transfer then content, each in reverse.
/// Sets ec as decoder::write, or to the coding fault if the body is
/// incomplete.
BCT_API system::data_chunk decode(const system::data_slice& body,
    const codings& value, size_t limit, code& ec) NOEXCEPT;

/// The response coding preferred by the request (rfc9110 12.5.3), by quality
/// then gzip, deflate and identity, or unknown if none is acceptable.
BCT_API coding accepted_coding(const fields& request) NOEXCEPT;

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
