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
#ifndef LIBBITCOIN_NETWORK_IMPL_IO_OPERATIONS_IPP
#define LIBBITCOIN_NETWORK_IMPL_IO_OPERATIONS_IPP

#include <memory>
#include <vector>
#include <bitcoin/network/error.hpp>

namespace libbitcoin {
namespace network {
namespace io {

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

namespace detail {

/// Drop the leading count bytes from the buffers.
template <typename Buffer>
inline void consume(std::vector<Buffer>& buffers, size_t count) NOEXCEPT
{
    auto it = buffers.begin();
    for (; it != buffers.end() && count >= it->size(); ++it)
        count -= it->size();

    buffers.erase(buffers.begin(), it);
    if (!buffers.empty())
        buffers.front() += count;
}

template <typename Buffer>
inline bool exhausted(const std::vector<Buffer>& buffers) NOEXCEPT
{
    for (const auto& buffer: buffers)
        if (is_nonzero(buffer.size()))
            return false;

    return true;
}

/// The state of a composed transfer, which repeats until all bytes move.
template <typename Stream, typename Buffer, bool Write>
class transfer
  : public std::enable_shared_from_this<transfer<Stream, Buffer, Write>>
{
public:
    transfer(Stream& stream, std::vector<Buffer>&& buffers,
        transfer_handler&& handler) NOEXCEPT
      : stream_(stream), buffers_(std::move(buffers)),
        handler_(std::move(handler))
    {
    }

    void start() NOEXCEPT
    {
        // Nothing to transfer completes on the executor.
        if (exhausted(buffers_))
        {
            stream_.get_executor().post([self = this->shared_from_this()]()
                NOEXCEPT
            {
                self->handler_(error::success, self->total_);
            });

            return;
        }

        next();
    }

private:
    void next() NOEXCEPT
    {
        auto completion = [self = this->shared_from_this()](const code& ec,
            size_t size) NOEXCEPT
        {
            self->handle(ec, size);
        };

        if constexpr (Write)
            stream_.async_write_some(buffers_, std::move(completion));
        else
            stream_.async_read_some(buffers_, std::move(completion));
    }

    void handle(const code& ec, size_t size) NOEXCEPT
    {
        total_ += size;
        consume(buffers_, size);
        if (ec || exhausted(buffers_))
        {
            handler_(ec, total_);
            return;
        }

        next();
    }

    Stream& stream_;
    std::vector<Buffer> buffers_;
    const transfer_handler handler_;
    size_t total_{};
};

} // namespace detail

template <typename Stream, typename Buffers>
void async_write(Stream& stream, const Buffers& buffers,
    transfer_handler&& handler) NOEXCEPT
{
    using operation = detail::transfer<Stream, const_buffer, true>;
    std::make_shared<operation>(stream, to_buffers<const_buffer>(buffers),
        std::move(handler))->start();
}

template <typename Stream, typename Buffers>
void async_read(Stream& stream, const Buffers& buffers,
    transfer_handler&& handler) NOEXCEPT
{
    using operation = detail::transfer<Stream, mutable_buffer, false>;
    std::make_shared<operation>(stream, to_buffers<mutable_buffer>(buffers),
        std::move(handler))->start();
}

template <typename Stream, typename Buffers>
size_t write(Stream& stream, const Buffers& buffers, code& ec) NOEXCEPT
{
    auto remaining = to_buffers<const_buffer>(buffers);
    size_t total{};
    ec = error::success;
    while (!detail::exhausted(remaining))
    {
        const auto size = stream.write_some(remaining, ec);
        total += size;
        if (ec)
            break;

        detail::consume(remaining, size);
    }

    return total;
}

template <typename Stream, typename Buffers>
size_t read(Stream& stream, const Buffers& buffers, code& ec) NOEXCEPT
{
    auto remaining = to_buffers<mutable_buffer>(buffers);
    size_t total{};
    ec = error::success;
    while (!detail::exhausted(remaining))
    {
        const auto size = stream.read_some(remaining, ec);
        total += size;
        if (ec)
            break;

        detail::consume(remaining, size);
    }

    return total;
}

template <typename Stream>
size_t read_until(Stream& stream, std::string& text,
    const std::string_view& delimiter, code& ec) NOEXCEPT
{
    ec = error::success;
    std::string chunk{};
    size_t searched{};
    while (true)
    {
        const auto start = system::floored_subtract(searched,
            delimiter.size());
        const auto position = text.find(delimiter, start);
        if (position != std::string::npos)
            return position + delimiter.size();

        searched = text.size();
        chunk.resize(std::max(text.capacity(), delimiter.size()));

        const auto size = stream.read_some(buffer(chunk), ec);
        if (ec)
            return zero;

        text.append(chunk.data(), size);
    }
}

BC_POP_WARNING()

} // namespace io
} // namespace network
} // namespace libbitcoin

#endif
