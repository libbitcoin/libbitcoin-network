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
#include <bitcoin/network/ws/reader.hpp>

#include <stdexcept>
#include <bitcoin/network/define.hpp>
#include <bitcoin/network/ws/frame.hpp>

namespace libbitcoin {
namespace network {
namespace ws {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

reader::reader(size_t maximum, bool server) NOEXCEPT
  : maximum_(possible_wide_cast<uint64_t>(maximum)), server_(server)
{
}

size_t reader::put(const std::span<const uint8_t>& data,
    http::flat_buffer& message, event& out, code& ec) NOEXCEPT
{
    out = event::none;
    ec = error::success;
    size_t used{};
    while (true)
    {
        const auto rest = data.subspan(used);
        const auto step = payload_ ? put_payload(rest, message, out, ec) :
            put_header(rest, message, out, ec);

        used += step;
        if (ec || out != event::none)
            return used;

        if (is_zero(step))
        {
            ec = error::need_more;
            return used;
        }
    }
}

bool reader::binary() const NOEXCEPT
{
    return binary_;
}

const data_chunk& reader::control() const NOEXCEPT
{
    return control_;
}

// private
// ----------------------------------------------------------------------------

size_t reader::put_header(const std::span<const uint8_t>& data,
    http::flat_buffer& message, event& out, code& ec) NOEXCEPT
{
    const auto size = parse_header(frame_, data, server_, ec);
    if (ec)
    {
        if (ec == error::need_more)
            ec = error::success;

        return {};
    }

    switch (frame_.code)
    {
        case opcode::continuation:
            if (!continuing_)
            {
                ec = error::bad_continuation;
                return {};
            }

            break;
        case opcode::text:
        case opcode::binary:
            if (continuing_)
            {
                ec = error::bad_data_frame;
                return {};
            }

            binary_ = frame_.code == opcode::binary;
            utf8_ = {};
            size_ = zero;
            break;
        default:
            control_.clear();
            break;
    }

    const auto control = to_value(frame_.code) >= to_value(opcode::close);
    if (!control && frame_.length > maximum_ - std::min(maximum_, size_))
    {
        ec = error::message_too_big;
        return {};
    }

    offset_ = zero;
    payload_ = true;

    // A frame without payload is complete.
    if (is_zero(frame_.length))
        put_payload({}, message, out, ec);

    return size;
}

size_t reader::put_payload(const std::span<const uint8_t>& data,
    http::flat_buffer& message, event& out, code& ec) NOEXCEPT
{
    const auto remaining = frame_.length - offset_;
    const auto control = to_value(frame_.code) >= to_value(opcode::close);

    if (control)
    {
        if (possible_wide_cast<uint64_t>(data.size()) < remaining)
            return {};

        const auto size = possible_narrow_cast<size_t>(remaining);
        control_.assign(data.begin(), std::next(data.begin(), size));
        if (frame_.masked)
            apply_mask(control_, frame_.key, zero);

        payload_ = false;
        switch (frame_.code)
        {
            case opcode::ping:
                out = event::ping;
                break;
            case opcode::pong:
                out = event::pong;
                break;
            default:
            {
                uint16_t value{};
                std::string reason{};
                if (parse_close(value, reason, control_, ec))
                    out = event::close;

                break;
            }
        }

        return size;
    }

    const auto size = possible_narrow_cast<size_t>(std::min(remaining,
        possible_wide_cast<uint64_t>(data.size())));

    if (!is_zero(size))
    {
        try
        {
            const auto buffer = message.prepare(size);
            const std::span<uint8_t> target{ pointer_cast<uint8_t>(
                buffer.data()), size };

            std::copy_n(data.begin(), size, target.begin());
            if (frame_.masked)
                apply_mask(target, frame_.key, offset_);

            if (!binary_ && !utf8_.write(target))
            {
                ec = error::bad_frame_payload;
                return {};
            }

            message.commit(size);
        }
        catch (const std::length_error&)
        {
            ec = error::websocket_buffer_overflow;
            return {};
        }
    }

    offset_ += possible_wide_cast<uint64_t>(size);
    size_ += possible_wide_cast<uint64_t>(size);
    if (offset_ < frame_.length)
        return size;

    payload_ = false;
    continuing_ = !frame_.fin;
    if (frame_.fin)
    {
        if (!binary_ && !utf8_.finish())
        {
            ec = error::bad_frame_payload;
            return size;
        }

        out = event::message;
    }

    return size;
}

BC_POP_WARNING()

} // namespace ws
} // namespace network
} // namespace libbitcoin
