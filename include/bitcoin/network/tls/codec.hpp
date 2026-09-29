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
#ifndef LIBBITCOIN_NETWORK_TLS_CODEC_HPP
#define LIBBITCOIN_NETWORK_TLS_CODEC_HPP

#include <vector>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace tls {

/// Big-endian reader of TLS presentation language (rfc8446 3). A failed read
/// invalidates the reader, and all later reads return empty.
class BCT_API reader
{
public:
    reader(const const_byte_span& data) NOEXCEPT;

    /// True if no read has failed.
    operator bool() const NOEXCEPT;

    /// True if no read has failed and all data has been read.
    bool is_complete() const NOEXCEPT;

    uint8_t read_8() NOEXCEPT;
    uint16_t read_16() NOEXCEPT;
    uint32_t read_24() NOEXCEPT;
    const_byte_span read_bytes(size_t size) NOEXCEPT;

    /// Length-prefixed vectors (prefix of 1, 2 or 3 bytes).
    const_byte_span read_vector_8() NOEXCEPT;
    const_byte_span read_vector_16() NOEXCEPT;
    const_byte_span read_vector_24() NOEXCEPT;

    /// Length-prefixed lists of 16 bit integers (prefix of 1 or 2 bytes).
    std::vector<uint16_t> read_list_8() NOEXCEPT;
    std::vector<uint16_t> read_list_16() NOEXCEPT;

protected:
    std::vector<uint16_t> read_list(const const_byte_span& bytes) NOEXCEPT;
    void invalidate() NOEXCEPT;

private:
    const_byte_span data_;
    bool valid_;
};

/// Big-endian writer of TLS presentation language.
class BCT_API writer
{
public:
    const system::data_chunk& data() const NOEXCEPT;

    void write_8(uint8_t value) NOEXCEPT;
    void write_16(uint16_t value) NOEXCEPT;
    void write_24(uint32_t value) NOEXCEPT;
    void write_bytes(const const_byte_span& bytes) NOEXCEPT;

    /// Length-prefixed vectors (prefix of 1, 2 or 3 bytes).
    void write_vector_8(const const_byte_span& bytes) NOEXCEPT;
    void write_vector_16(const const_byte_span& bytes) NOEXCEPT;
    void write_vector_24(const const_byte_span& bytes) NOEXCEPT;

private:
    system::data_chunk data_{};
};

} // namespace tls
} // namespace network
} // namespace libbitcoin

#endif
