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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_BODIES_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_BODIES_HPP

#include <span>
#include <bitcoin/network/io/io.hpp>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/http/file.hpp>
#include <bitcoin/network/messages/http/flat_buffer.hpp>
#include <bitcoin/network/messages/http/header.hpp>
#include <bitcoin/network/messages/http/message.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// A body that must be empty.
struct empty_body
{
    struct value_type
    {
    };

    struct reader
    {
        template <bool IsRequest>
        reader(header<IsRequest>&, value_type&) NOEXCEPT
        {
        }

        void init(const length_type&, code& ec) NOEXCEPT
        {
            ec.clear();
        }

        size_t put(const io::const_buffer& buffer, code& ec) NOEXCEPT
        {
            using namespace network::error;
            if (is_zero(buffer.size()))
                ec.clear();
            else
                ec = to_http_code(http_error_t::unexpected_body);

            return {};
        }

        void finish(code& ec) NOEXCEPT
        {
            ec.clear();
        }
    };

    struct writer
    {
        using const_buffers_type = io::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest>
        writer(header<IsRequest>&, value_type&) NOEXCEPT
        {
        }

        void init(code& ec) NOEXCEPT
        {
            ec.clear();
        }

        out_buffer get(code& ec) NOEXCEPT
        {
            ec.clear();
            return {};
        }
    };
};

/// A body of bytes or characters in a contiguous container.
template <typename Container>
struct contiguous_body
{
    using value_type = Container;

    struct reader
    {
        template <bool IsRequest>
        reader(header<IsRequest>&, value_type& value) NOEXCEPT
          : value_(value)
        {
        }

        void init(const length_type& length, code& ec) NOEXCEPT
        {
            using namespace network::error;
            BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
            if (length.has_value() && length.value() > value_.max_size())
            {
                ec = to_http_code(http_error_t::buffer_overflow);
                return;
            }

            value_.clear();
            if (length.has_value())
                value_.reserve(system::possible_narrow_cast<size_t>(
                    length.value()));
            BC_POP_WARNING()

            ec.clear();
        }

        size_t put(const io::const_buffer& buffer, code& ec) NOEXCEPT
        {
            using namespace system;
            using namespace network::error;
            using element = typename value_type::value_type;

            BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
            if (buffer.size() > value_.max_size() - value_.size())
            {
                ec = to_http_code(http_error_t::buffer_overflow);
                return {};
            }

            const auto data = pointer_cast<const element>(buffer.data());
            value_.insert(value_.end(), data, std::next(data, buffer.size()));
            BC_POP_WARNING()

            ec.clear();
            return buffer.size();
        }

        void finish(code& ec) NOEXCEPT
        {
            ec.clear();
        }

    private:
        value_type& value_;
    };

    struct writer
    {
        using const_buffers_type = io::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest>
        writer(header<IsRequest>&, value_type& value) NOEXCEPT
          : value_(value)
        {
        }

        void init(code& ec) NOEXCEPT
        {
            ec.clear();
        }

        out_buffer get(code& ec) NOEXCEPT
        {
            ec.clear();
            return out_buffer{ std::make_pair(const_buffers_type{
                value_.data(), value_.size() }, false) };
        }

    private:
        value_type& value_;
    };
};

/// A body of bytes.
using chunk_body = contiguous_body<system::data_chunk>;

/// A body of characters.
using string_body = contiguous_body<std::string>;

/// A body of bytes in caller-owned memory (written in one buffer, or read up
/// to its size).
struct span_body
{
    using value_type = std::span<uint8_t>;

    struct reader
    {
        template <bool IsRequest>
        reader(header<IsRequest>&, value_type& value) NOEXCEPT
          : value_(value)
        {
        }

        void init(const length_type& length, code& ec) NOEXCEPT
        {
            using namespace network::error;
            if (length.has_value() && length.value() > value_.size())
                ec = to_http_code(http_error_t::buffer_overflow);
            else
                ec.clear();
        }

        size_t put(const io::const_buffer& buffer, code& ec) NOEXCEPT
        {
            using namespace system;
            using namespace network::error;
            if (buffer.size() > value_.size())
            {
                ec = to_http_code(http_error_t::buffer_overflow);
                return {};
            }

            const auto data = pointer_cast<const uint8_t>(buffer.data());
            std::copy_n(data, buffer.size(), value_.begin());
            value_ = value_.subspan(buffer.size());
            ec.clear();
            return buffer.size();
        }

        void finish(code& ec) NOEXCEPT
        {
            ec.clear();
        }

    private:
        value_type& value_;
    };

    struct writer
    {
        using const_buffers_type = io::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest>
        writer(header<IsRequest>&, value_type& value) NOEXCEPT
          : value_(value)
        {
        }

        void init(code& ec) NOEXCEPT
        {
            ec.clear();
        }

        out_buffer get(code& ec) NOEXCEPT
        {
            ec.clear();
            return out_buffer{ std::make_pair(const_buffers_type{
                value_.data(), value_.size() }, false) };
        }

    private:
        value_type& value_;
    };
};

/// A body of caller-provided buffers, each followed by a need_buffer while
/// more remain to be provided.
struct buffer_body
{
    struct value_type
    {
        /// The bytes, null when none are provided.
        void* data{};

        /// The number of bytes.
        size_t size{};

        /// More bytes follow (in subsequent buffers).
        bool more{ true };
    };

    struct reader
    {
        template <bool IsRequest>
        reader(header<IsRequest>&, value_type& value) NOEXCEPT
          : value_(value)
        {
        }

        void init(const length_type&, code& ec) NOEXCEPT
        {
            ec.clear();
        }

        size_t put(const io::const_buffer& buffer, code& ec) NOEXCEPT
        {
            using namespace system;
            using namespace network::error;
            if (is_null(value_.data))
            {
                ec = to_http_code(http_error_t::need_buffer);
                return {};
            }

            const auto size = std::min(buffer.size(), value_.size);
            std::copy_n(pointer_cast<const uint8_t>(buffer.data()), size,
                pointer_cast<uint8_t>(value_.data));

            value_.data = std::next(pointer_cast<uint8_t>(value_.data), size);
            value_.size -= size;
            if (size < buffer.size())
                ec = to_http_code(http_error_t::need_buffer);
            else
                ec.clear();

            return size;
        }

        void finish(code& ec) NOEXCEPT
        {
            ec.clear();
        }

    private:
        value_type& value_;
    };

    struct writer
    {
        using const_buffers_type = io::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest>
        writer(header<IsRequest>&, value_type& value) NOEXCEPT
          : value_(value)
        {
        }

        void init(code& ec) NOEXCEPT
        {
            ec.clear();
        }

        out_buffer get(code& ec) NOEXCEPT
        {
            using namespace system;
            using namespace network::error;
            if (toggle_)
            {
                toggle_ = false;
                if (value_.more)
                    ec = to_http_code(http_error_t::need_buffer);
                else
                    ec.clear();

                return {};
            }

            if (!is_null(value_.data))
            {
                ec.clear();
                toggle_ = true;
                return out_buffer{ std::make_pair(const_buffers_type{
                    value_.data, value_.size }, value_.more) };
            }

            if (value_.more)
                ec = to_http_code(http_error_t::need_buffer);
            else
                ec.clear();

            return {};
        }

    private:
        value_type& value_;
        bool toggle_{};
    };
};

/// A body serialized by the caller into the retained writer buffer (written
/// in one buffer), or into owned memory when larger than the buffer limit.
struct stream_body
{
    struct value_type
    {
        /// Serializes exactly size bytes into the provided memory.
        std::function<bool(const system::data_slab&)> write{};

        /// The serialized size.
        size_t size{};

        /// Writer serialization buffer, owned by the socket.
        mutable flat_buffer* buffer{};
    };

    struct writer
    {
        using const_buffers_type = io::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest>
        writer(header<IsRequest>&, value_type& value) NOEXCEPT
          : value_(value)
        {
        }

        void init(code& ec) NOEXCEPT
        {
            using namespace system;
            using namespace network::error;
            if (!value_.write)
            {
                ec = to_http_code(http_error_t::unexpected_body);
                return;
            }

            try
            {
                uint8_t* data{};
                const auto size = value_.size;
                if (!is_null(value_.buffer) &&
                    size <= value_.buffer->max_size())
                {
                    value_.buffer->clear();
                    data = pointer_cast<uint8_t>(
                        value_.buffer->prepare(size).data());
                }
                else
                {
                    owned_ = std::make_unique_for_overwrite<uint8_t[]>(size);
                    data = owned_.get();
                }

                if (!value_.write({ data, std::next(data, size) }))
                {
                    ec = to_http_code(http_error_t::unexpected_body);
                    return;
                }

                bytes_ = { data, size };
                ec.clear();
            }
            catch (...)
            {
                ec = to_http_code(http_error_t::bad_alloc);
            }
        }

        out_buffer get(code& ec) NOEXCEPT
        {
            ec.clear();
            return out_buffer{ std::make_pair(bytes_, false) };
        }

    private:
        value_type& value_;
        std::unique_ptr<uint8_t[]> owned_{};
        const_buffers_type bytes_{};
    };
};

/// A body read from (written by) a file.
struct file_body
{
    using value_type = file;

    /// The size of each read from the file.
    static constexpr size_t read_size = 4096;

    struct reader
    {
        template <bool IsRequest>
        reader(header<IsRequest>&, value_type& value) NOEXCEPT
          : value_(value)
        {
        }

        void init(const length_type&, code& ec) NOEXCEPT
        {
            using namespace network::error;
            if (value_.is_open())
                ec.clear();
            else
                ec = error::bad_stream;
        }

        size_t put(const io::const_buffer& buffer, code& ec) NOEXCEPT
        {
            using namespace system;
            using namespace network::error;
            if (!value_.write(pointer_cast<const uint8_t>(buffer.data()),
                buffer.size()))
            {
                ec = error::bad_stream;
                return {};
            }

            ec.clear();
            return buffer.size();
        }

        void finish(code& ec) NOEXCEPT
        {
            ec.clear();
        }

    private:
        value_type& value_;
    };

    struct writer
    {
        using const_buffers_type = io::const_buffer;
        using out_buffer = get_buffer<const_buffers_type>;

        template <bool IsRequest>
        writer(header<IsRequest>&, value_type& value) NOEXCEPT
          : value_(value)
        {
        }

        void init(code& ec) NOEXCEPT
        {
            using namespace network::error;
            if (!value_.is_open())
            {
                ec = error::bad_stream;
                return;
            }

            remaining_ = value_.size();
            BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
            buffer_.resize(read_size);
            BC_POP_WARNING()
            ec.clear();
        }

        out_buffer get(code& ec) NOEXCEPT
        {
            using namespace system;
            using namespace network::error;
            const auto size = limit<size_t>(std::min<uint64_t>(remaining_,
                buffer_.size()));

            size_t read{};
            if (!value_.read(read, buffer_.data(), size) || read != size)
            {
                ec = error::bad_stream;
                return {};
            }

            remaining_ -= read;
            ec.clear();
            return out_buffer{ std::make_pair(const_buffers_type{
                buffer_.data(), read }, !is_zero(remaining_)) };
        }

    private:
        value_type& value_;
        system::data_chunk buffer_{};
        uint64_t remaining_{};
    };
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
