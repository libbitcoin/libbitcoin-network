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
#ifndef LIBBITCOIN_NETWORK_MESSAGES_HTTP_FILE_HPP
#define LIBBITCOIN_NETWORK_MESSAGES_HTTP_FILE_HPP

#include <filesystem>
#include <fstream>
#include <memory>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

/// Not thread safe, non-virtual.
/// A binary file, opened for reading or (truncated) writing.
class BCT_API file
{
public:
    DELETE_COPY(file);
    DEFAULT_MOVE(file);

    file() NOEXCEPT = default;
    ~file() NOEXCEPT;

    /// Open for reading, false if not opened.
    bool open_read(const std::filesystem::path& path) NOEXCEPT;

    /// Open for writing (truncated), false if not opened.
    bool open_write(const std::filesystem::path& path) NOEXCEPT;

    /// The file is open.
    bool is_open() const NOEXCEPT;

    /// The size of a file opened for reading.
    uint64_t size() const NOEXCEPT;

    /// Read up to size bytes into data, returning the number read, false if
    /// the read fails.
    bool read(size_t& out, uint8_t* data, size_t size) NOEXCEPT;

    /// Write size bytes, false if the write fails.
    bool write(const uint8_t* data, size_t size) NOEXCEPT;

    /// Close the file.
    void close() NOEXCEPT;

private:
    std::unique_ptr<std::fstream> stream_{};
    uint64_t size_{};
};

} // namespace http
} // namespace network
} // namespace libbitcoin

#endif
