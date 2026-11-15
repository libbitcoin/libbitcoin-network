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
#ifndef LIBBITCOIN_NETWORK_IMPL_MESSAGES_HTTP_SERIALIZER_IPP
#define LIBBITCOIN_NETWORK_IMPL_MESSAGES_HTTP_SERIALIZER_IPP

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

#define TEMPLATE template <bool IsRequest, class Body>
#define CLASS serializer<IsRequest, Body>

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

TEMPLATE
CLASS::serializer(message_type& message) NOEXCEPT
  : message_(message),
    writer_(message.base(), message.body()),
    chunked_(message.chunked())
{
}

// The header is yielded with the first body buffers, so that a message of one
// body buffer is written in one operation.
TEMPLATE
const_buffers CLASS::next(code& ec) NOEXCEPT
{
    ec.clear();
    const_buffers buffers{};
    switch (state_)
    {
        case state::header:
        {
            codings_ = to_codings(message_.base(), ec);
            if (ec)
                return {};

            if (!codings_.empty())
                frame_coded();

            header_ = serialize_header(message_.base());
            writer_.init(ec);
            if (ec)
                return {};

            state_ = state::body;
            buffers.emplace_back(header_.data(), header_.size());
            append_body(buffers, ec);
            return ec ? const_buffers{} : buffers;
        }
        case state::body:
        {
            append_body(buffers, ec);
            return ec ? const_buffers{} : buffers;
        }
        case state::done:
        default:
            return {};
    }
}

// A message without a body is not coded. The size of a coded body is not
// known in advance, so a content-length is replaced by chunking (http 1.1),
// or by closing the connection.
TEMPLATE
void CLASS::frame_coded() NOEXCEPT
{
    const auto length = message_.content_length();
    auto bodiless = length.has_value() && is_zero(length.value());
    if constexpr (!IsRequest)
        bodiless = bodiless || message_.bodiless();

    if (bodiless)
    {
        message_.erase(field::content_encoding);
        codings_ = {};
        return;
    }

    if (!chunked_ && message_.content_length().has_value())
    {
        if (message_.version() >= version_1_1)
        {
            message_.chunked(true);
            chunked_ = true;
        }
        else
        {
            message_.content_length({});
            message_.keep_alive(false);
        }
    }

    encoder_.emplace(codings_);
}

// Each part written is encoded, the writer pausing (need_buffer) as it may.
TEMPLATE
void CLASS::append_coded(const_buffers& buffers, code& ec) NOEXCEPT
{
    using namespace system;
    coded_.clear();
    while (coded_.empty() && !written_)
    {
        const auto out = writer_.get(ec);
        if (ec)
            return;

        if (out.has_value())
        {
            const auto& data = out.value().first;
            const auto begin = pointer_cast<const uint8_t>(data.data());
            encoder_->write(coded_, data_slice(begin, std::next(begin,
                data.size())));
        }

        if (!out.has_value() || !out.value().second)
        {
            encoder_->flush(coded_);
            written_ = true;
        }
    }

    append(buffers, { coded_.data(), coded_.size() }, !written_);
}

TEMPLATE
void CLASS::append_body(const_buffers& buffers, code& ec) NOEXCEPT
{
    if (encoder_.has_value())
    {
        append_coded(buffers, ec);
        return;
    }

    const auto out = writer_.get(ec);
    if (ec)
        return;

    const auto more = out.has_value() && out.value().second;
    const auto data = out.has_value() ? out.value().first :
        io::const_buffer{};

    append(buffers, data, more);
}

TEMPLATE
void CLASS::append(const_buffers& buffers, const io::const_buffer& data,
    bool more) NOEXCEPT
{
    if (!more)
        state_ = state::done;

    if (!chunked_)
    {
        if (!is_zero(data.size()))
            buffers.push_back(data);

        return;
    }

    if (!is_zero(data.size()))
    {
        prefix_ = chunk_header(data.size());
        buffers.emplace_back(prefix_.data(), prefix_.size());
        buffers.push_back(data);
        buffers.emplace_back(chunk_end.data(), chunk_end.size());
    }

    if (!more)
        buffers.emplace_back(last_chunk.data(), last_chunk.size());
}

TEMPLATE
bool CLASS::is_header_done() const NOEXCEPT
{
    return state_ != state::header;
}

TEMPLATE
bool CLASS::is_done() const NOEXCEPT
{
    return state_ == state::done;
}

TEMPLATE
bool CLASS::binary() const NOEXCEPT
{
    return writer_.binary();
}

TEMPLATE
typename CLASS::message_type& CLASS::get() NOEXCEPT
{
    return message_;
}

#undef CLASS
#undef TEMPLATE

#define TEMPLATE template <bool IsRequest, class Body>
#define CLASS body_serializer<IsRequest, Body>

TEMPLATE
CLASS::body_serializer(message_type& message) NOEXCEPT
  : writer_(message.base(), message.body())
{
}

TEMPLATE
const_buffers CLASS::next(code& ec) NOEXCEPT
{
    ec.clear();
    if (done_)
        return {};

    if (!initialized_)
    {
        initialized_ = true;
        writer_.init(ec);
        if (ec)
            return {};
    }

    const auto out = writer_.get(ec);
    if (ec)
        return {};

    done_ = !out.has_value() || !out.value().second;
    return { out.has_value() ? out.value().first : io::const_buffer{} };
}

TEMPLATE
bool CLASS::is_done() const NOEXCEPT
{
    return done_;
}

TEMPLATE
bool CLASS::binary() const NOEXCEPT
{
    return writer_.binary();
}

BC_POP_WARNING()

#undef CLASS
#undef TEMPLATE

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
