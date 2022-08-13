/**
 * Copyright (c) 2011-2021 libbitcoin developers (see AUTHORS)
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
#include "test.hpp"

#include <filesystem>
#include <iostream>
#include <bitcoin/system.hpp>

// copied from libbitcoin-system-test

namespace std {

std::ostream& operator<<(std::ostream& stream,
    const system::data_slice& slice) noexcept
{
    // Avoid serialize() here for its own test benefit.
    // stream << serialize(slice);
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    stream << encode_base16(slice);
    BC_POP_WARNING()
    return stream;
}

} // namespace std

namespace test {

const std::string directory = "tests";

// C++17: use std::filesystem.
bool clear(const std::filesystem::path& file_directory) noexcept
{
    // C++17: use std::filesystem.
    // remove_all returns count removed, and error code if fails.
    // create_directories returns true if path exists or created.
    // used for setup, with no expectations of file/directory existence.
    const auto path = system::to_extended_path(file_directory);
    boost::system::error_code ec;
    std::filesystem::remove_all(path, ec);
    return !ec && std::filesystem::create_directories(path, ec);
}

// C++17: use std::filesystem.
bool create(const std::filesystem::path& file_path) noexcept
{
    // Creates and returns true if file already existed (and no error).
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::ofstream file(system::to_extended_path(file_path));
    return file.good();
    BC_POP_WARNING()
}

// C++17: use std::filesystem.
bool exists(const std::filesystem::path& file_path) noexcept
{
    // Returns true only if file existed.
    BC_PUSH_WARNING(NO_THROW_IN_NOEXCEPT)
    std::ifstream file(system::to_extended_path(file_path));
    return file.good();
    BC_POP_WARNING()
}

// C++17: use std::filesystem.
bool remove(const std::filesystem::path& file_path) noexcept
{
    // C++17: use std::filesystem.
    // Deletes and returns false if file did not exist (or error).
    boost::system::error_code ec;
    return std::filesystem::remove(system::to_extended_path(file_path), ec);
}

} // namespace test