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
#include <bitcoin/network/noise/stream.hpp>

#include <algorithm>
#include <bitcoin/network/async/async.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/sv2.hpp>

namespace libbitcoin {
namespace network {
namespace noise {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

constexpr auto protocol_error = error::protocol_violation;

static bool payload_length(size_t& out, const const_byte_span& heading) NOEXCEPT
{
    const auto value = messages::sv2::heading::deserialize(
        heading.first(cipher::heading_size));

    if (!value)
        return false;

    out = value->message_length;
    return true;
}

// Constructors.
// ----------------------------------------------------------------------------

stream::stream(io::socket&& socket, const context& context) NOEXCEPT
  : socket_(std::move(socket)), context_(&context)
{
}

stream::stream(io::socket&& socket) NOEXCEPT
  : socket_(std::move(socket)), context_(nullptr)
{
}

// Properties.
// ----------------------------------------------------------------------------

stream::executor_type stream::get_executor() NOEXCEPT
{
    return socket_.get_executor();
}

io::socket& stream::next_layer() NOEXCEPT
{
    return socket_;
}

const io::socket& stream::next_layer() const NOEXCEPT
{
    return socket_;
}

const cipher::key& stream::static_key() const NOEXCEPT
{
    return static_key_;
}

const cipher::certificate& stream::certificate() const NOEXCEPT
{
    return certificate_;
}

// Handshake.
// ----------------------------------------------------------------------------

void stream::async_handshake(result_handler&& handler) NOEXCEPT
{
    if (is_null(context_))
        do_initiate(std::move(handler));
    else
        do_respond(std::move(handler));
}

void stream::do_respond(const result_handler& handler) NOEXCEPT
{
    cipher::certificate proof{};
    if (!context_->valid() || !context_->issue(proof, unix_time()))
    {
        io::post(get_executor(),
            std::bind(handler, code{ error::operation_failed }));
        return;
    }

    cipher_.emplace(context_->static_secret(), context_->static_key(), proof);
    io::async_read(socket_, io::buffer(act1_),
        std::bind(&stream::handle_act1,
            this, _1, handler));
}

void stream::handle_act1(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    if (!cipher_->respond(act2_, act1_))
    {
        handler(protocol_error);
        return;
    }

    io::async_write(socket_, io::buffer(act2_),
        std::bind(handler, _1));
}

void stream::do_initiate(const result_handler& handler) NOEXCEPT
{
    cipher_.emplace();
    cipher_->initiate(act1_);
    io::async_write(socket_, io::buffer(act1_),
        std::bind(&stream::handle_act1_sent,
            this, _1, handler));
}

void stream::handle_act1_sent(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    io::async_read(socket_, io::buffer(act2_),
        std::bind(&stream::handle_act2,
            this, _1, handler));
}

void stream::handle_act2(const code& ec,
    const result_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    handler(cipher_->complete(static_key_, certificate_, act2_) ?
        error::success : protocol_error);
}

// Read.
// ----------------------------------------------------------------------------

void stream::do_read_some(const byte_span& out,
    const io_handler& handler) NOEXCEPT
{
    if (out.empty() || offset_ < received_.size())
    {
        io::post(get_executor(),
            std::bind(handler, code{ error::success }, take(out)));
        return;
    }

    input_.resize(cipher::sealed_heading_size);
    io::async_read(socket_, io::buffer(input_),
        std::bind(&stream::handle_heading,
            this, _1, out, handler));
}

// Copy received frame bytes to out, releasing the frame once read.
size_t stream::take(const byte_span& out) NOEXCEPT
{
    const auto size = std::min(out.size(), received_.size() - offset_);
    std::copy_n(std::next(received_.begin(), offset_), size, out.begin());
    offset_ += size;
    if (offset_ == received_.size())
    {
        received_.clear();
        offset_ = zero;
    }

    return size;
}

void stream::handle_heading(const code& ec, const byte_span& out,
    const io_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec, zero);
        return;
    }

    size_t length{};
    received_.resize(cipher::heading_size);
    if (!cipher_->decrypt_heading(received_, input_) ||
        !payload_length(length, received_))
    {
        handler(protocol_error, zero);
        return;
    }

    if (is_zero(length))
    {
        handler(error::success, take(out));
        return;
    }

    input_.resize(cipher::sealed_size(length));
    io::async_read(socket_, io::buffer(input_),
        std::bind(&stream::handle_payload,
            this, _1, length, out, handler));
}

void stream::handle_payload(const code& ec, size_t length,
    const byte_span& out, const io_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec, zero);
        return;
    }

    received_.resize(cipher::heading_size + length);
    const auto payload = std::span{ received_ }.subspan(cipher::heading_size);
    if (!cipher_->decrypt_payload(payload, input_))
    {
        handler(protocol_error, zero);
        return;
    }

    handler(error::success, take(out));
}

// Write.
// ----------------------------------------------------------------------------

// Seal each whole frame of the written bytes, retaining a partial frame.
bool stream::seal(const const_byte_span& data) NOEXCEPT
{
    pending_.insert(pending_.cend(), data.begin(), data.end());
    while (pending_.size() >= cipher::heading_size)
    {
        size_t length{};
        if (!payload_length(length, pending_))
            return false;

        const auto frame = cipher::heading_size + length;
        if (pending_.size() < frame)
            return true;

        const auto start = output_.size();
        const auto sealed = cipher::sealed_heading_size +
            cipher::sealed_size(length);

        output_.resize(start + sealed);
        const auto out = std::span{ output_ }.subspan(start);
        if (!cipher_->encrypt(out, std::span{ pending_ }.first(frame)))
            return false;

        pending_.erase(pending_.begin(), std::next(pending_.begin(), frame));
    }

    return true;
}

void stream::do_write_some(const const_byte_span& data,
    const io_handler& handler) NOEXCEPT
{
    if (data.empty())
    {
        io::post(get_executor(),
            std::bind(handler, code{ error::success }, zero));
        return;
    }

    if (!seal(data))
    {
        io::post(get_executor(), std::bind(handler, protocol_error, zero));
        return;
    }

    const auto size = data.size();
    flush([handler, size](const code& ec) NOEXCEPT
    {
        handler(ec, ec ? zero : size);
    });
}

size_t stream::do_write(const const_byte_span& data, code& ec) NOEXCEPT
{
    ec = error::success;
    if (writing_)
    {
        ec = error::operation_failed;
        return zero;
    }

    if (data.empty())
        return zero;

    if (!seal(data))
    {
        ec = protocol_error;
        return zero;
    }

    io::write(socket_, io::buffer(output_), ec);
    output_.clear();
    return ec ? zero : data.size();
}

// Shutdown.
// ----------------------------------------------------------------------------

void stream::async_shutdown(result_handler&& handler) NOEXCEPT
{
    flush(std::move(handler));
}

// Transport.
// ----------------------------------------------------------------------------

// The handler completes once all output preceding it is written.
void stream::flush(const result_handler& handler) NOEXCEPT
{
    waiting_.push_back(handler);
    if (!writing_)
        write_next();
}

void stream::write_next() NOEXCEPT
{
    std::vector<result_handler> handlers{};
    handlers.swap(waiting_);

    if (output_.empty())
    {
        for (auto& handler: handlers)
            io::post(get_executor(),
                std::bind(std::move(handler), code{ error::success }));

        return;
    }

    writing_ = true;
    sending_.swap(output_);
    output_.clear();
    io::async_write(socket_, io::buffer(sending_),
        std::bind(&stream::handle_write,
            this, _1, std::move(handlers)));
}

void stream::handle_write(const code& ec,
    const std::vector<result_handler>& handlers) NOEXCEPT
{
    writing_ = false;
    sending_.clear();
    for (const auto& handler: handlers)
        handler(ec);

    if (!writing_ && !waiting_.empty())
        write_next();
}

BC_POP_WARNING()

} // namespace noise
} // namespace network
} // namespace libbitcoin
