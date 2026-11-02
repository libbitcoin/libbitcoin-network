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
#include <bitcoin/network/ws/frame.hpp>

#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace ws {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

constexpr uint8_t fin_bit = 0x80;
constexpr uint8_t reserved_bits = 0x70;
constexpr uint8_t opcode_bits = 0x0f;
constexpr uint8_t mask_bit = 0x80;
constexpr uint8_t length_bits = 0x7f;
constexpr uint8_t length16 = 126;
constexpr uint8_t length64 = 127;

static bool is_control(opcode code) NOEXCEPT
{
    return to_value(code) >= to_value(opcode::close);
}

static bool is_defined(uint8_t value) NOEXCEPT
{
    switch (static_cast<opcode>(value))
    {
        case opcode::continuation:
        case opcode::text:
        case opcode::binary:
        case opcode::close:
        case opcode::ping:
        case opcode::pong:
            return true;
        default:
            return false;
    }
}

size_t parse_header(frame_header& out, const std::span<const uint8_t>& data,
    bool server, code& ec) NOEXCEPT
{
    if (data.size() < two)
    {
        ec = error::need_more;
        return {};
    }

    const auto first = data[0];
    const auto second = data[1];
    if (is_nonzero(first & reserved_bits))
    {
        ec = error::bad_reserved_bits;
        return {};
    }

    if (!is_defined(first & opcode_bits))
    {
        ec = error::bad_opcode;
        return {};
    }

    out.fin = is_nonzero(first & fin_bit);
    out.code = static_cast<opcode>(first & opcode_bits);
    out.masked = is_nonzero(second & mask_bit);
    if (is_control(out.code) && !out.fin)
    {
        ec = error::bad_control_fragment;
        return {};
    }

    const auto length = second & length_bits;
    if (is_control(out.code) && is_greater(length, maximum_control))
    {
        ec = error::bad_control_size;
        return {};
    }

    if (server && !out.masked)
    {
        ec = error::bad_unmasked_frame;
        return {};
    }

    if (!server && out.masked)
    {
        ec = error::bad_masked_frame;
        return {};
    }

    const auto extended = length == length16 ? two :
        (length == length64 ? sizeof(uint64_t) : zero);

    const auto size = two + extended + (out.masked ? sizeof(mask) : zero);
    if (data.size() < size)
    {
        ec = error::need_more;
        return {};
    }

    out.length = length;
    if (!is_zero(extended))
    {
        out.length = zero;
        for (size_t index{}; index < extended; ++index)
            out.length = (out.length << byte_bits) | data[two + index];

        // Lengths use the shortest encoding and the high bit is clear.
        if ((length == length16 && out.length < length16) ||
            (length == length64 && (out.length <= max_uint16 ||
            is_nonzero(out.length >> 63))))
        {
            ec = error::bad_size;
            return {};
        }
    }

    if (out.masked)
        std::copy_n(std::next(data.begin(), two + extended), sizeof(mask),
            out.key.begin());

    ec = error::success;
    return size;
}

data_chunk serialize_header(opcode code, bool fin, uint64_t length,
    const std::optional<mask>& key) NOEXCEPT
{
    data_chunk out{};
    out.reserve(maximum_header);
    out.push_back(possible_narrow_and_sign_cast<uint8_t>(
        (fin ? fin_bit : 0x00) | to_value(code)));

    const uint8_t masked = key.has_value() ? mask_bit : 0x00;
    if (length < length16)
    {
        out.push_back(possible_narrow_cast<uint8_t>(masked | length));
    }
    else if (length <= max_uint16)
    {
        out.push_back(masked | length16);
        out.push_back(possible_narrow_cast<uint8_t>(length >> byte_bits));
        out.push_back(possible_narrow_cast<uint8_t>(length));
    }
    else
    {
        out.push_back(masked | length64);
        for (auto shift = 56; shift >= 0; shift -= byte_bits)
            out.push_back(possible_narrow_cast<uint8_t>(length >> shift));
    }

    if (key.has_value())
        out.insert(out.end(), key->begin(), key->end());

    return out;
}

void apply_mask(const std::span<uint8_t>& data, const mask& key,
    uint64_t offset) NOEXCEPT
{
    const auto start = possible_narrow_cast<size_t>(offset %
        possible_wide_cast<uint64_t>(sizeof(mask)));

    for (size_t index{}; index < data.size(); ++index)
        data[index] ^= key[(start + index) % sizeof(mask)];
}

bool is_valid_close_code(uint16_t value) NOEXCEPT
{
    switch (value)
    {
        case 1000: case 1001: case 1002: case 1003:
        case 1007: case 1008: case 1009: case 1010:
        case 1011: case 1012: case 1013:
            return true;
        case 1004: case 1005: case 1006: case 1014: case 1015:
            return false;
        default:
            return value > 999 && (value < 1016 || value > 2999);
    }
}

bool parse_close(uint16_t& value, std::string& reason,
    const std::span<const uint8_t>& payload, code& ec) NOEXCEPT
{
    value = zero;
    reason.clear();
    if (payload.empty())
    {
        ec = error::success;
        return true;
    }

    if (payload.size() == one)
    {
        ec = error::bad_close_size;
        return false;
    }

    value = possible_narrow_and_sign_cast<uint16_t>((payload[0] << byte_bits) |
        payload[1]);

    if (!is_valid_close_code(value))
    {
        ec = error::bad_close_code;
        return false;
    }

    const auto text = payload.subspan(two);
    utf8_validator validator{};
    if (!validator.write(text) || !validator.finish())
    {
        ec = error::bad_close_payload;
        return false;
    }

    reason.assign(text.begin(), text.end());
    ec = error::success;
    return true;
}

data_chunk close_payload(close_code value) NOEXCEPT
{
    const auto number = to_value(value);
    return
    {
        possible_narrow_and_sign_cast<uint8_t>(number >> byte_bits),
        possible_narrow_cast<uint8_t>(number)
    };
}

// utf8_validator
// ----------------------------------------------------------------------------

bool utf8_validator::write(const std::span<const uint8_t>& data) NOEXCEPT
{
    for (const auto byte: data)
    {
        if (is_zero(needed_))
        {
            lower_ = 0x80;
            upper_ = 0xbf;
            if (byte <= 0x7f)
                continue;

            if (byte >= 0xc2 && byte <= 0xdf)
            {
                needed_ = 1;
            }
            else if (byte >= 0xe0 && byte <= 0xef)
            {
                needed_ = 2;
                if (byte == 0xe0) lower_ = 0xa0;
                if (byte == 0xed) upper_ = 0x9f;
            }
            else if (byte >= 0xf0 && byte <= 0xf4)
            {
                needed_ = 3;
                if (byte == 0xf0) lower_ = 0x90;
                if (byte == 0xf4) upper_ = 0x8f;
            }
            else
            {
                return false;
            }

            continue;
        }

        if (byte < lower_ || byte > upper_)
            return false;

        lower_ = 0x80;
        upper_ = 0xbf;
        --needed_;
    }

    return true;
}

bool utf8_validator::finish() NOEXCEPT
{
    const auto complete = is_zero(needed_);
    needed_ = zero;
    return complete;
}

BC_POP_WARNING()

} // namespace ws
} // namespace network
} // namespace libbitcoin
