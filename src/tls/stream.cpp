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
#include <bitcoin/network/tls/stream.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tls/constants.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

using namespace system;
using namespace std::placeholders;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

// Constructor.
// ----------------------------------------------------------------------------

stream::stream(asio::socket&& socket, const context& context) NOEXCEPT
  : socket_(std::move(socket)),
    context_(context),
    server_(context),
    input_(record_header_size + maximum_ciphertext)
{
}

// Properties.
// ----------------------------------------------------------------------------

stream::executor_type stream::get_executor() NOEXCEPT
{
    return socket_.get_executor();
}

asio::socket& stream::next_layer() NOEXCEPT
{
    return socket_;
}

const asio::socket& stream::next_layer() const NOEXCEPT
{
    return socket_;
}

// Handshake.
// ----------------------------------------------------------------------------

void stream::async_handshake(result_handler&& handler) NOEXCEPT
{
    if (!context_.is_ready())
    {
        const boost_code ec{ code{ error::tls_alert_internal_error } };
        boost::asio::post(get_executor(), std::bind(std::move(handler), ec));
        return;
    }

    receive(std::bind(&stream::handle_handshake,
        this, _1, std::move(handler)));
}

void stream::handle_handshake(const boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    if (server_.is_failed())
    {
        handler(failure());
        return;
    }

    if (server_.is_established())
    {
        handler({});
        return;
    }

    receive(std::bind(&stream::handle_handshake,
        this, _1, handler));
}

// Read.
// ----------------------------------------------------------------------------

void stream::do_read_some(const byte_span& out,
    const io_handler& handler) NOEXCEPT
{
    boost_code ec{};
    size_t size{};
    if (try_read(ec, size, out))
    {
        boost::asio::post(get_executor(), std::bind(handler, ec, size));
        return;
    }

    receive(std::bind(&stream::handle_read_some,
        this, _1, out, handler));
}

// True if the read is complete without receiving.
bool stream::try_read(boost_code& ec, size_t& size,
    const byte_span& out) NOEXCEPT
{
    ec = {};
    size = zero;
    if (out.empty())
        return true;

    if (is_nonzero(server_.readable()))
    {
        size = server_.read(out);
        return true;
    }

    if (server_.is_closed())
    {
        ec = boost::asio::error::eof;
        return true;
    }

    if (server_.is_failed())
    {
        ec = failure();
        return true;
    }

    return false;
}

void stream::handle_read_some(const boost_code& ec, const byte_span& out,
    const io_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec, zero);
        return;
    }

    boost_code result{};
    size_t size{};
    if (try_read(result, size, out))
    {
        handler(result, size);
        return;
    }

    receive(std::bind(&stream::handle_read_some,
        this, _1, out, handler));
}

// Write.
// ----------------------------------------------------------------------------

void stream::do_write_some(const const_byte_span& data,
    const io_handler& handler) NOEXCEPT
{
    if (data.empty())
    {
        boost::asio::post(get_executor(),
            std::bind(handler, boost_code{}, zero));
        return;
    }

    const auto size = std::min(data.size(), maximum_plaintext);
    if (!server_.write(data.first(size)))
    {
        const auto ec = server_.is_failed() ? failure() :
            boost::asio::error::shut_down;

        boost::asio::post(get_executor(), std::bind(handler, ec, zero));
        return;
    }

    flush([handler, size](const boost_code& ec) NOEXCEPT
    {
        handler(ec, ec ? zero : size);
    });
}

size_t stream::do_write(const const_byte_span& data, boost_code& ec) NOEXCEPT
{
    ec = {};
    if (writing_)
    {
        ec = boost::asio::error::in_progress;
        return zero;
    }

    if (data.empty())
        return zero;

    const auto size = std::min(data.size(), maximum_plaintext);
    if (!server_.write(data.first(size)))
    {
        ec = server_.is_failed() ? failure() : boost::asio::error::shut_down;
        return zero;
    }

    auto& output = server_.output();
    boost::asio::write(socket_, boost::asio::buffer(output), ec);
    output.clear();
    return ec ? zero : size;
}

// Shutdown.
// ----------------------------------------------------------------------------

void stream::async_shutdown(result_handler&& handler) NOEXCEPT
{
    if (server_.is_failed())
    {
        boost::asio::post(get_executor(),
            std::bind(std::move(handler), boost_code{}));
        return;
    }

    server_.close();
    flush(std::bind(&stream::handle_close_sent,
        this, _1, std::move(handler)));
}

void stream::handle_close_sent(const boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    handle_close_received(ec, handler);
}

void stream::handle_close_received(const boost_code& ec,
    const result_handler& handler) NOEXCEPT
{
    if (ec)
    {
        handler(ec);
        return;
    }

    if (server_.is_failed())
    {
        handler(failure());
        return;
    }

    if (server_.is_closed())
    {
        handler({});
        return;
    }

    receive(std::bind(&stream::handle_close_received,
        this, _1, handler));
}

// Transport.
// ----------------------------------------------------------------------------

boost_code stream::failure() const NOEXCEPT
{
    return code{ error::tls_to_error_code(server_.failure()) };
}

// Closure of the transport without close_notify truncates the stream.
boost_code stream::transport(const boost_code& ec) const NOEXCEPT
{
    if (ec == boost::asio::error::eof && !server_.is_closed())
        return code{ error::tls_stream_truncated };

    return ec;
}

// A receive joins any receive in progress, as they share the input buffer.
void stream::receive(result_handler&& handler) NOEXCEPT
{
    receivers_.push_back(std::move(handler));
    if (receiving_)
        return;

    receiving_ = true;
    socket_.async_read_some(boost::asio::buffer(input_),
        std::bind(&stream::handle_receive,
            this, _1, _2));
}

// Records produced by receipt (alerts and key updates) are sent first.
void stream::handle_receive(const boost_code& ec, size_t size) NOEXCEPT
{
    receiving_ = false;
    std::vector<result_handler> handlers{};
    handlers.swap(receivers_);

    if (ec)
    {
        const auto result = transport(ec);
        for (const auto& handler: handlers)
            handler(result);

        return;
    }

    server_.receive({ input_.data(), size });
    if (server_.output().empty())
    {
        for (const auto& handler: handlers)
            handler({});

        return;
    }

    for (const auto& handler: handlers)
        flush(handler);
}

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

    auto& output = server_.output();
    if (output.empty())
    {
        for (auto& handler: handlers)
            boost::asio::post(get_executor(),
                std::bind(std::move(handler), boost_code{}));

        return;
    }

    writing_ = true;
    sending_.swap(output);
    output.clear();
    boost::asio::async_write(socket_, boost::asio::buffer(sending_),
        std::bind(&stream::handle_write,
            this, _1, std::move(handlers)));
}

void stream::handle_write(const boost_code& ec,
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

} // namespace tls
} // namespace network
} // namespace libbitcoin
