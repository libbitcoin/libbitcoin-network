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
#include <bitcoin/network/messages/http/file.hpp>

#include <filesystem>
#include <fstream>
#include <bitcoin/network/define.hpp>

namespace libbitcoin {
namespace network {
namespace http {

using namespace system;

BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)

file::~file() NOEXCEPT
{
    close();
}

bool file::open_read(const std::filesystem::path& path) NOEXCEPT
{
    try
    {
        close();
        stream_.open(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (!stream_.is_open())
            return false;

        size_ = possible_narrow_sign_cast<uint64_t>(
            static_cast<std::streamoff>(stream_.tellg()));

        stream_.seekg(0);
        return stream_.good();
    }
    catch (...)
    {
        return false;
    }
}

bool file::open_write(const std::filesystem::path& path) NOEXCEPT
{
    try
    {
        close();
        stream_.open(path, std::ios::out | std::ios::binary |
            std::ios::trunc);
        size_ = zero;
        return stream_.is_open();
    }
    catch (...)
    {
        return false;
    }
}

bool file::is_open() const NOEXCEPT
{
    return stream_.is_open();
}

uint64_t file::size() const NOEXCEPT
{
    return size_;
}

bool file::read(size_t& out, uint8_t* data, size_t size) NOEXCEPT
{
    try
    {
        stream_.read(pointer_cast<char>(data),
            possible_narrow_sign_cast<std::streamsize>(size));

        out = possible_narrow_sign_cast<size_t>(stream_.gcount());
        return !stream_.bad();
    }
    catch (...)
    {
        out = zero;
        return false;
    }
}

bool file::write(const uint8_t* data, size_t size) NOEXCEPT
{
    try
    {
        stream_.write(pointer_cast<const char>(data),
            possible_narrow_sign_cast<std::streamsize>(size));

        return stream_.good();
    }
    catch (...)
    {
        return false;
    }
}

void file::close() NOEXCEPT
{
    try
    {
        if (stream_.is_open())
            stream_.close();
    }
    catch (...)
    {
    }
}

BC_POP_WARNING()

} // namespace http
} // namespace network
} // namespace libbitcoin
