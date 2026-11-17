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
#include <bitcoin/network/tor/cell.hpp>

#include <bitcoin/network/define.hpp>
#include <bitcoin/network/tor/constants.hpp>

// tor-spec 3 (cell packet format)

namespace libbitcoin {
namespace network {
namespace tor {

using namespace system;

data_chunk encode(const cell& value, bool narrow) NOEXCEPT
{
    BC_ASSERT(is_variable(value.command) ||
        (value.payload.size() <= payload_size));

    data_chunk out{};
    write::bytes::data sink(out);
    if (narrow)
        sink.write_2_bytes_big_endian(narrow_cast<uint16_t>(value.circuit));
    else
        sink.write_4_bytes_big_endian(value.circuit);

    sink.write_byte(value.command);
    if (is_variable(value.command))
    {
        const auto size = value.payload.size();
        sink.write_2_bytes_big_endian(possible_narrow_cast<uint16_t>(size));
        sink.write_bytes(value.payload);
    }
    else
    {
        sink.write_bytes(value.payload);
        sink.write_bytes(data_chunk(payload_size - value.payload.size()));
    }

    sink.flush();
    return out;
}

size_t decode(cell& out, const_byte_span data, bool narrow) NOEXCEPT
{
    read::bytes::copy source({ data.begin(), data.end() });
    const uint32_t circuit = narrow ? source.read_2_bytes_big_endian() :
        source.read_4_bytes_big_endian();
    const auto command = source.read_byte();

    size_t length{ payload_size };
    if (is_variable(command))
        length = source.read_2_bytes_big_endian();

    auto payload = source.read_bytes(length);
    if (!source)
        return zero;

    // Circuit id, command and any length precede payload.
    const auto circuit_bytes = narrow ? narrow_circuit_size : circuit_size;
    const auto length_bytes = is_variable(command) ? sizeof(uint16_t) : zero;

    out = { circuit, command, std::move(payload) };
    return circuit_bytes + sizeof(command) + length_bytes + length;
}

} // namespace tor
} // namespace network
} // namespace libbitcoin
