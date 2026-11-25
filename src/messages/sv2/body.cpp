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
#include <bitcoin/network/messages/sv2/body.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/messages/sv2/registry.hpp>
#include <bitcoin/network/messages/sv2/sv2.hpp>

namespace libbitcoin {
namespace network {
namespace messages {
namespace sv2 {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
BC_PUSH_WARNING(NO_UNGUARDED_POINTERS)
BC_PUSH_WARNING(NO_POINTER_ARITHMETIC)

// sv2::body::reader
// ----------------------------------------------------------------------------
// Reader parses at most one message per read, taking the heading and then the
// payload once each is buffered whole by caller. Fault member carries parse
// failure detail (returned code is generic).

static size_t set_fault(frame& value, const code& fault,
    code& ec) NOEXCEPT
{
    value.fault = fault;
    ec = error::to_http_code(error::http_error_t::bad_value);
    return {};
}

void body::reader::init(const http::length_type&, code& ec) NOEXCEPT
{
    need_ = heading::size();
    headed_ = false;
    done_ = false;
    value_.fault = error::success;
    value_.payload.reset();
    ec = {};
}

// private
bool body::reader::accept(const std::span<const uint8_t>& payload,
    code& ec) NOEXCEPT
{
    const auto index = value_.head.index();
    if (index != registry::unknown)
    {
        value_.payload = registry::to_any(index, payload);

        if (!value_.payload)
        {
            set_fault(value_, error::invalid_message, ec);
            return false;
        }
    }

    need_ = zero;
    done_ = true;
    return true;
}

size_t body::reader::put(const buffer_type& buffer, code& ec) NOEXCEPT
{
    auto data = pointer_cast<const uint8_t>(buffer.data());
    auto size = buffer.size();
    size_t used{};
    ec = {};

    if (!headed_)
    {
        if (size < heading::size())
            return value_.exact && !is_zero(size) ?
                set_fault(value_, error::invalid_heading, ec) : zero;

        const data_slice head{ data, std::next(data, heading::size()) };
        system::stream::in::fast stream{ head };
        system::read::bytes::fast source{ stream };
        value_.head = heading::deserialize(source);

        if (!source)
            return set_fault(value_, error::invalid_heading, ec);

        const auto index = value_.head.index();
        if (index != registry::unknown &&
            registry::channels().at(index) != value_.head.channel())
            return set_fault(value_, error::invalid_heading, ec);

        if (value_.head.message_length > value_.maximum)
            return set_fault(value_, error::oversized_payload, ec);

        used = heading::size();
        data = std::next(data, used);
        size -= used;
        headed_ = true;
        need_ = value_.head.message_length;

        if (value_.exact && size != need_)
            return set_fault(value_, error::invalid_heading, ec);
    }

    // The payload is parsed in place (buffered by the caller).
    if (size < need_)
        return used;

    const auto length = need_;
    return accept({ data, length }, ec) ? used + length : zero;
}

void body::reader::finish(code& ec) NOEXCEPT
{
    ec = done_ ? code{ error::success } :
        error::to_http_code(error::http_error_t::need_more);
}

bool body::reader::done() const NOEXCEPT
{
    return done_;
}

// sv2::body::writer
// ----------------------------------------------------------------------------
// Writer translates typed message to its serialized frame, in retained
// buffer when frame fits within its bound.

void body::writer::init(code& ec) NOEXCEPT
{
    done_ = false;
    const auto size = ceilinged_add(heading::size(), value_.size);
    if (!is_null(value_.buffer) && size <= floored_subtract(
        value_.buffer->max_size(), value_.buffer->size()))
    {
        try
        {
            const auto data = pointer_cast<uint8_t>(
                value_.buffer->prepare(size).data());

            if (!registry::to_frame(value_.index, value_.message,
                { data, std::next(data, size) }))
            {
                ec = error::to_http_code(error::http_error_t::bad_value);
                return;
            }

            bytes_ = { data, size };
            ec = error::success;
        }
        catch (...)
        {
            ec = error::to_http_code(error::http_error_t::bad_alloc);
        }

        return;
    }

    value_.data = registry::to_frame(value_.index, value_.message);

    if (!value_.data)
    {
        ec = error::to_http_code(error::http_error_t::bad_value);
        return;
    }

    bytes_ = { value_.data->data(), value_.data->size() };
    ec = error::success;
}

body::writer::out_buffer body::writer::get(code& ec) NOEXCEPT
{
    ec = {};

    if (done_)
        return {};

    done_ = true;
    return out_buffer{ std::make_pair(bytes_, false) };
}

bool body::writer::done() const NOEXCEPT
{
    return done_;
}

BC_POP_WARNING()
BC_POP_WARNING()
BC_POP_WARNING()

} // namespace sv2
} // namespace messages
} // namespace network
} // namespace libbitcoin
